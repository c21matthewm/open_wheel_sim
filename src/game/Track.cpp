#include "game/Track.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "config/ConfigFile.h"

namespace sim {
namespace {

constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0F * kPi;

float smoothStep5(float value) {
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return clamped * clamped * clamped * (clamped * (clamped * 6.0F - 15.0F) + 10.0F);
}

}  // namespace

const char* trackSurfaceName(TrackSurface surface) {
    switch (surface) {
        case TrackSurface::Asphalt:
            return "ASPHALT";
        case TrackSurface::Apron:
            return "APRON";
        case TrackSurface::Grass:
            return "GRASS";
    }
    return "UNKNOWN";
}

void TrackConfig::load(const ConfigFile& config) {
    name = config.getString("track.name", name);
    straightLengthM =
        std::max(100.0F, config.getFloat("geometry.straight_length_m", straightLengthM));
    shortChuteLengthM =
        std::max(50.0F, config.getFloat("geometry.short_chute_length_m", shortChuteLengthM));
    cornerRadiusM =
        std::max(50.0F, config.getFloat("geometry.corner_radius_m", cornerRadiusM));
    roadWidthM = std::max(6.0F, config.getFloat("geometry.road_width_m", roadWidthM));
    apronWidthM = std::max(0.0F, config.getFloat("geometry.apron_width_m", apronWidthM));
    outerWallGapM =
        std::max(0.2F, config.getFloat("geometry.outer_wall_gap_m", outerWallGapM));
    innerWallGapM =
        std::max(0.2F, config.getFloat("geometry.inner_wall_gap_m", innerWallGapM));
    outerWallHeightM =
        std::max(0.2F, config.getFloat("geometry.outer_wall_height_m", outerWallHeightM));
    bankingDegrees =
        std::clamp(config.getFloat("geometry.banking_degrees", bankingDegrees), 0.0F, 20.0F);
    bankTransitionFraction = std::clamp(
        config.getFloat("geometry.bank_transition_fraction", bankTransitionFraction), 0.01F, 0.45F);
    bankTransitionRunoutM = std::clamp(
        config.getFloat("geometry.bank_transition_runout_m", bankTransitionRunoutM), 5.0F, 160.0F);
    spawnDistanceBeforeStartM = std::max(
        1.0F, config.getFloat("timing.spawn_distance_before_start_m", spawnDistanceBeforeStartM));
    asphaltGripMultiplier =
        std::max(0.05F, config.getFloat("surfaces.asphalt_grip_multiplier", asphaltGripMultiplier));
    apronGripMultiplier =
        std::max(0.05F, config.getFloat("surfaces.apron_grip_multiplier", apronGripMultiplier));
    grassGripMultiplier =
        std::max(0.05F, config.getFloat("surfaces.grass_grip_multiplier", grassGripMultiplier));
    grassLongitudinalGripMultiplier = std::max(
        grassGripMultiplier,
        config.getFloat("surfaces.grass_longitudinal_grip_multiplier", grassLongitudinalGripMultiplier));
    asphaltResistanceMultiplier = std::max(
        0.1F,
        config.getFloat("surfaces.asphalt_resistance_multiplier", asphaltResistanceMultiplier));
    apronResistanceMultiplier = std::max(
        0.1F, config.getFloat("surfaces.apron_resistance_multiplier", apronResistanceMultiplier));
    grassResistanceMultiplier = std::max(
        0.1F, config.getFloat("surfaces.grass_resistance_multiplier", grassResistanceMultiplier));
    wallRestitution =
        std::clamp(config.getFloat("barrier.restitution", wallRestitution), 0.0F, 0.8F);
    renderSegments = std::clamp(config.getInt("render.segments", renderSegments), 64, 2048);
}

Track::Track(TrackConfig config) : config_(std::move(config)) {
    halfStraightLengthM_ = config_.straightLengthM * 0.5F;
    lapLengthM_ = 2.0F * config_.straightLengthM +
                  2.0F * config_.shortChuteLengthM +
                  kTwoPi * config_.cornerRadiusM;
}

TrackPose Track::poseAtDistance(float distanceM) const {
    TrackPose pose;
    pose.distanceM = normalizeDistance(distanceM);

    const float radius = config_.cornerRadiusM;
    const float shortHalf = config_.shortChuteLengthM * 0.5F;
    const float cornerLength = kPi * 0.5F * radius;
    const float frontFirstEnd = halfStraightLengthM_;
    const float turn4End = frontFirstEnd + cornerLength;
    const float northChuteEnd = turn4End + config_.shortChuteLengthM;
    const float turn3End = northChuteEnd + cornerLength;
    const float backStretchEnd = turn3End + config_.straightLengthM;
    const float turn2End = backStretchEnd + cornerLength;
    const float southChuteEnd = turn2End + config_.shortChuteLengthM;
    const float turn1End = southChuteEnd + cornerLength;

    if (pose.distanceM < frontFirstEnd) {
        pose.centerX = shortHalf + radius;
        pose.centerZ = pose.distanceM;
        pose.tangentX = 0.0F;
        pose.tangentZ = 1.0F;
    } else if (pose.distanceM < turn4End) {
        const float arcAngle = (pose.distanceM - frontFirstEnd) / radius;
        const float angle = arcAngle;
        pose.centerX = shortHalf + radius * std::cos(angle);
        pose.centerZ = halfStraightLengthM_ + radius * std::sin(angle);
        pose.tangentX = -std::sin(angle);
        pose.tangentZ = std::cos(angle);
    } else if (pose.distanceM < northChuteEnd) {
        pose.centerX = shortHalf - (pose.distanceM - turn4End);
        pose.centerZ = halfStraightLengthM_ + radius;
        pose.tangentX = -1.0F;
        pose.tangentZ = 0.0F;
    } else if (pose.distanceM < turn3End) {
        const float arcAngle = (pose.distanceM - northChuteEnd) / radius;
        const float angle = kPi * 0.5F + arcAngle;
        pose.centerX = -shortHalf + radius * std::cos(angle);
        pose.centerZ = halfStraightLengthM_ + radius * std::sin(angle);
        pose.tangentX = -std::sin(angle);
        pose.tangentZ = std::cos(angle);
    } else if (pose.distanceM < backStretchEnd) {
        pose.centerX = -shortHalf - radius;
        pose.centerZ = halfStraightLengthM_ - (pose.distanceM - turn3End);
        pose.tangentX = 0.0F;
        pose.tangentZ = -1.0F;
    } else if (pose.distanceM < turn2End) {
        const float arcAngle = (pose.distanceM - backStretchEnd) / radius;
        const float angle = kPi + arcAngle;
        pose.centerX = -shortHalf + radius * std::cos(angle);
        pose.centerZ = -halfStraightLengthM_ + radius * std::sin(angle);
        pose.tangentX = -std::sin(angle);
        pose.tangentZ = std::cos(angle);
    } else if (pose.distanceM < southChuteEnd) {
        pose.centerX = -shortHalf + (pose.distanceM - turn2End);
        pose.centerZ = -halfStraightLengthM_ - radius;
        pose.tangentX = 1.0F;
        pose.tangentZ = 0.0F;
    } else if (pose.distanceM < turn1End) {
        const float arcAngle = (pose.distanceM - southChuteEnd) / radius;
        const float angle = kPi * 1.5F + arcAngle;
        pose.centerX = shortHalf + radius * std::cos(angle);
        pose.centerZ = -halfStraightLengthM_ + radius * std::sin(angle);
        pose.tangentX = -std::sin(angle);
        pose.tangentZ = std::cos(angle);
    } else {
        pose.centerX = shortHalf + radius;
        pose.centerZ = -halfStraightLengthM_ + (pose.distanceM - turn1End);
        pose.tangentX = 0.0F;
        pose.tangentZ = 1.0F;
    }

    pose.rightX = pose.tangentZ;
    pose.rightZ = -pose.tangentX;
    pose.yawRadians = std::atan2(pose.tangentX, pose.tangentZ);
    pose.bankRadians = bankForDistance(pose.distanceM);
    pose.roadHalfWidthM = roadHalfWidthM();
    pose.centerHeightM = roadHalfWidthM() * std::tan(pose.bankRadians);
    return pose;
}

TrackPose Track::spawnPose() const {
    return poseAtDistance(lapLengthM_ - config_.spawnDistanceBeforeStartM);
}

TrackSample Track::sample(float positionX, float positionZ) const {
    TrackSample sample;
    const float radius = config_.cornerRadiusM;
    const float shortHalf = config_.shortChuteLengthM * 0.5F;
    const float cornerLength = kPi * 0.5F * radius;
    const float frontFirstEnd = halfStraightLengthM_;
    const float turn4End = frontFirstEnd + cornerLength;
    const float northChuteEnd = turn4End + config_.shortChuteLengthM;
    const float turn3End = northChuteEnd + cornerLength;
    const float backStretchEnd = turn3End + config_.straightLengthM;
    const float turn2End = backStretchEnd + cornerLength;
    const float southChuteEnd = turn2End + config_.shortChuteLengthM;
    const float turn1End = southChuteEnd + cornerLength;
    float progress = 0.0F;

    if (positionX > shortHalf && positionZ > halfStraightLengthM_) {
        const float angle = std::clamp(
            std::atan2(positionZ - halfStraightLengthM_, positionX - shortHalf), 0.0F, kPi * 0.5F);
        progress = frontFirstEnd + angle * radius;
    } else if (positionX < -shortHalf && positionZ > halfStraightLengthM_) {
        const float angle = std::clamp(
            std::atan2(positionZ - halfStraightLengthM_, positionX + shortHalf), kPi * 0.5F, kPi);
        progress = northChuteEnd + (angle - kPi * 0.5F) * radius;
    } else if (positionX < -shortHalf && positionZ < -halfStraightLengthM_) {
        float angle = std::atan2(positionZ + halfStraightLengthM_, positionX + shortHalf);
        if (angle < 0.0F) {
            angle += kTwoPi;
        }
        angle = std::clamp(angle, kPi, kPi * 1.5F);
        progress = backStretchEnd + (angle - kPi) * radius;
    } else if (positionX > shortHalf && positionZ < -halfStraightLengthM_) {
        float angle = std::atan2(positionZ + halfStraightLengthM_, positionX - shortHalf);
        if (angle < 0.0F) {
            angle += kTwoPi;
        }
        angle = std::clamp(angle, kPi * 1.5F, kTwoPi);
        progress = southChuteEnd + (angle - kPi * 1.5F) * radius;
    } else {
        struct StraightProjection {
            float progress = 0.0F;
            float distanceSquared = 0.0F;
        };
        const auto squared = [](float value) { return value * value; };
        const auto frontProjection = [&]() {
            const float clampedZ =
                std::clamp(positionZ, -halfStraightLengthM_, halfStraightLengthM_);
            const float progressOnFront = clampedZ >= 0.0F
                                              ? clampedZ
                                              : turn1End + clampedZ + halfStraightLengthM_;
            return StraightProjection{
                progressOnFront,
                squared(positionX - (shortHalf + radius)) + squared(positionZ - clampedZ)};
        };
        const auto northProjection = [&]() {
            const float clampedX = std::clamp(positionX, -shortHalf, shortHalf);
            return StraightProjection{
                turn4End + (shortHalf - clampedX),
                squared(positionX - clampedX) + squared(positionZ - (halfStraightLengthM_ + radius))};
        };
        const auto backProjection = [&]() {
            const float clampedZ =
                std::clamp(positionZ, -halfStraightLengthM_, halfStraightLengthM_);
            return StraightProjection{
                turn3End + (halfStraightLengthM_ - clampedZ),
                squared(positionX - (-shortHalf - radius)) + squared(positionZ - clampedZ)};
        };
        const auto southProjection = [&]() {
            const float clampedX = std::clamp(positionX, -shortHalf, shortHalf);
            return StraightProjection{
                turn2End + (clampedX + shortHalf),
                squared(positionX - clampedX) + squared(positionZ - (-halfStraightLengthM_ - radius))};
        };

        StraightProjection best = frontProjection();
        const StraightProjection candidates[] = {
            northProjection(),
            backProjection(),
            southProjection(),
        };
        for (const StraightProjection& candidate : candidates) {
            if (candidate.distanceSquared < best.distanceSquared) {
                best = candidate;
            }
        }
        progress = best.progress;
    }

    const TrackPose pose = poseAtDistance(progress);
    static_cast<TrackPose&>(sample) = pose;
    const float deltaX = positionX - pose.centerX;
    const float deltaZ = positionZ - pose.centerZ;
    sample.lateralOffsetM = deltaX * pose.rightX + deltaZ * pose.rightZ;

    const float roadHalfWidth = roadHalfWidthM();
    if (std::abs(sample.lateralOffsetM) <= roadHalfWidth) {
        sample.surface = TrackSurface::Asphalt;
        sample.gripMultiplier = config_.asphaltGripMultiplier;
        sample.longitudinalGripMultiplier = config_.asphaltGripMultiplier;
        sample.resistanceMultiplier = config_.asphaltResistanceMultiplier;
    } else if (
        sample.lateralOffsetM < -roadHalfWidth &&
        sample.lateralOffsetM >= -(roadHalfWidth + config_.apronWidthM)) {
        sample.surface = TrackSurface::Apron;
        sample.gripMultiplier = config_.apronGripMultiplier;
        sample.longitudinalGripMultiplier = config_.apronGripMultiplier;
        sample.resistanceMultiplier = config_.apronResistanceMultiplier;
    } else {
        sample.surface = TrackSurface::Grass;
        sample.gripMultiplier = config_.grassGripMultiplier;
        sample.longitudinalGripMultiplier = config_.grassLongitudinalGripMultiplier;
        sample.resistanceMultiplier = config_.grassResistanceMultiplier;
    }

    float heightOffset = sample.lateralOffsetM + roadHalfWidth;
    if (sample.lateralOffsetM < -roadHalfWidth) {
        heightOffset = 0.0F;
    } else {
        heightOffset = std::clamp(heightOffset, 0.0F, outerWallOffsetM() + roadHalfWidth);
    }
    sample.heightM = heightOffset * std::tan(pose.bankRadians);
    return sample;
}

float Track::bankForDistance(float distanceM) const {
    const float distance = normalizeDistance(distanceM);
    const float radius = config_.cornerRadiusM;
    const float cornerLength = kPi * 0.5F * radius;
    const float frontFirstEnd = halfStraightLengthM_;
    const float turn4End = frontFirstEnd + cornerLength;
    const float northChuteEnd = turn4End + config_.shortChuteLengthM;
    const float turn3End = northChuteEnd + cornerLength;
    const float backStretchEnd = turn3End + config_.straightLengthM;
    const float turn2End = backStretchEnd + cornerLength;
    const float southChuteEnd = turn2End + config_.shortChuteLengthM;
    const float turn1End = southChuteEnd + cornerLength;
    const float runout = std::clamp(config_.bankTransitionRunoutM, 1.0F, cornerLength * 0.49F);
    const float transitionLength = runout * 2.0F;

    const auto cornerBlend = [&](float entry, float exit) {
        const float rampIn = smoothStep5((distance - (entry - runout)) / transitionLength);
        const float rampOut = 1.0F - smoothStep5((distance - (exit - runout)) / transitionLength);
        return std::clamp(rampIn * rampOut, 0.0F, 1.0F);
    };

    const float blend = std::max(
        std::max(cornerBlend(frontFirstEnd, turn4End), cornerBlend(northChuteEnd, turn3End)),
        std::max(cornerBlend(backStretchEnd, turn2End), cornerBlend(southChuteEnd, turn1End)));
    return config_.bankingDegrees * kPi / 180.0F * blend;
}

float Track::normalizeDistance(float distanceM) const {
    float normalized = std::fmod(distanceM, lapLengthM_);
    if (normalized < 0.0F) {
        normalized += lapLengthM_;
    }
    return normalized;
}

}  // namespace sim
