#include "game/Track.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>
#include <utility>

#include "config/ConfigFile.h"

namespace sim {
static_assert(std::is_trivially_copyable_v<TrackPose>);
static_assert(std::is_trivially_copyable_v<TrackSample>);

namespace {

constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0F * kPi;
constexpr float kDegreesToRadians = kPi / 180.0F;

float smoothStep5(float value) {
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return clamped * clamped * clamped * (clamped * (clamped * 6.0F - 15.0F) + 10.0F);
}

float fract(float value) {
    return value - std::floor(value);
}

float triangle01(float value) {
    const float phase = fract(value);
    return 1.0F - std::abs(phase * 2.0F - 1.0F);
}

float deterministicAsphaltRoughness(float distanceM, float lateralOffsetM, float amplitudeM) {
    const float amp = std::clamp(amplitudeM, 0.0F, 0.020F);
    if (amp <= 0.0F) {
        return 0.0F;
    }
    const float waveA = std::sin(distanceM * (kTwoPi / 23.0F) + lateralOffsetM * 0.31F);
    const float waveB = std::sin(distanceM * (kTwoPi / 37.0F) - lateralOffsetM * 0.47F + 1.7F);
    const float waveC = std::sin(distanceM * (kTwoPi / 12.5F) + lateralOffsetM * 0.73F + 4.1F);
    return amp * (0.48F * waveA + 0.34F * waveB + 0.18F * waveC);
}

float ovalUndulation(float distanceM, float lateralOffsetM, float amplitudeM) {
    const float amp = std::clamp(amplitudeM, 0.0F, 0.030F);
    if (amp <= 0.0F) {
        return 0.0F;
    }
    const float waveA = std::sin(distanceM * (kTwoPi / 58.0F) + lateralOffsetM * 0.07F);
    const float waveB = std::sin(distanceM * (kTwoPi / 31.0F) - lateralOffsetM * 0.11F + 2.4F);
    return amp * (0.68F * waveA + 0.32F * waveB);
}

float gaussianBump(float distanceM, float widthM) {
    const float normalized = distanceM / std::max(0.05F, widthM);
    return std::exp(-normalized * normalized);
}

void assignRoadNormal(TrackSample& sample, float longitudinalSlope, float lateralSlope) {
    const float nx =
        -(sample.tangentX * longitudinalSlope + sample.rightX * lateralSlope);
    const float ny = 1.0F;
    const float nz =
        -(sample.tangentZ * longitudinalSlope + sample.rightZ * lateralSlope);
    const float length = std::max(0.001F, std::sqrt(nx * nx + ny * ny + nz * nz));
    sample.roadNormalX = nx / length;
    sample.roadNormalY = ny / length;
    sample.roadNormalZ = nz / length;
}

std::vector<float> parseFloatArray(const std::string& source) {
    std::vector<float> values;
    const char* cursor = source.c_str();
    while (*cursor != '\0') {
        while (*cursor != '\0' && !std::isdigit(static_cast<unsigned char>(*cursor)) &&
               *cursor != '-' && *cursor != '+') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        char* end = nullptr;
        const float value = std::strtof(cursor, &end);
        if (end == cursor) {
            ++cursor;
            continue;
        }
        values.push_back(value);
        cursor = end;
    }
    return values;
}

std::vector<std::array<float, 2>> parseFloatPairs(const std::string& source) {
    const std::vector<float> flat = parseFloatArray(source);
    std::vector<std::array<float, 2>> pairs;
    pairs.reserve(flat.size() / 2U);
    for (std::size_t index = 0; index + 1U < flat.size(); index += 2U) {
        pairs.push_back({flat[index], flat[index + 1U]});
    }
    return pairs;
}

std::vector<std::array<float, 2>> defaultHillElevationSpine() {
    return {
        {{0.0F, 0.0F}},       {{180.0F, 8.0F}},    {{320.0F, 12.0F}},
        {{420.0F, 8.5F}},     {{520.0F, 5.0F}},    {{600.0F, 3.5F}},
        {{720.0F, 4.0F}},     {{820.0F, 5.5F}},    {{900.0F, 5.0F}},
        {{980.0F, 5.5F}},     {{1040.0F, 6.0F}},   {{1130.0F, 6.5F}},
        {{1220.0F, 7.0F}},    {{1300.0F, 7.5F}},   {{1390.0F, 9.0F}},
        {{1470.0F, 11.5F}},   {{1540.0F, 14.0F}},  {{1590.0F, 16.0F}},
        {{1640.0F, 17.5F}},   {{1730.0F, 20.0F}},  {{1780.0F, 22.0F}},
        {{1830.0F, 22.0F}},   {{1870.0F, 16.0F}},  {{1910.0F, 10.5F}},
        {{1960.0F, 4.0F}},    {{2060.0F, 1.0F}},   {{2160.0F, 0.5F}},
        {{2240.0F, 1.5F}},    {{2320.0F, 3.0F}},   {{2390.0F, 4.5F}},
        {{2460.0F, 5.0F}},    {{2560.0F, 4.5F}},   {{2640.0F, 3.5F}},
        {{2720.0F, 2.0F}},    {{3150.0F, 1.0F}},   {{3600.0F, 0.0F}},
    };
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
        case TrackSurface::Curb:
            return "CURB";
    }
    return "UNKNOWN";
}

void TrackConfig::load(const ConfigFile& config) {
    name = config.getString("track.name", config.getString("name", name));
    type = config.getString("track.type", config.getString("track_type", type));
    lapLengthOverrideM = std::max(0.0F, config.getFloat("lap_length_m", lapLengthOverrideM));
    straightLengthM =
        std::max(100.0F, config.getFloat("geometry.straight_length_m", straightLengthM));
    shortChuteLengthM =
        std::max(50.0F, config.getFloat("geometry.short_chute_length_m", shortChuteLengthM));
    cornerRadiusM =
        std::max(50.0F, config.getFloat("geometry.corner_radius_m", cornerRadiusM));
    roadWidthM = std::max(6.0F, config.getFloat("geometry.road_width_m", config.getFloat("road_width_m", roadWidthM)));
    straightRoadWidthM =
        std::max(0.0F, config.getFloat("geometry.straight_road_width_m", straightRoadWidthM));
    turnRoadWidthM =
        std::max(0.0F, config.getFloat("geometry.turn_road_width_m", turnRoadWidthM));
    apronWidthM = std::max(0.0F, config.getFloat("geometry.apron_width_m", apronWidthM));
    curbWidthM = std::max(0.0F, config.getFloat("curb_width_m", curbWidthM));
    runoffWidthM = std::max(0.0F, config.getFloat("runoff_width_m", runoffWidthM));
    outerWallGapM =
        std::max(0.2F, config.getFloat("geometry.outer_wall_gap_m", outerWallGapM));
    innerWallGapM =
        std::max(0.2F, config.getFloat("geometry.inner_wall_gap_m", innerWallGapM));
    barrierOuterOffsetM =
        std::max(0.0F, config.getFloat("barrier_outer_offset_m", barrierOuterOffsetM));
    barrierInnerOffsetM =
        std::max(0.0F, config.getFloat("barrier_inner_offset_m", barrierInnerOffsetM));
    outerWallHeightM =
        std::max(0.2F, config.getFloat("geometry.outer_wall_height_m", outerWallHeightM));
    bankingDegrees =
        std::clamp(config.getFloat("geometry.banking_degrees", bankingDegrees), 0.0F, 20.0F);
    bankTransitionFraction = std::clamp(
        config.getFloat("geometry.bank_transition_fraction", bankTransitionFraction), 0.01F, 0.45F);
    bankTransitionRunoutM = std::clamp(
        config.getFloat("geometry.bank_transition_runout_m", bankTransitionRunoutM), 5.0F, 160.0F);
    pitLaneOffsetM =
        std::max(0.0F, config.getFloat("geometry.pit_lane_offset_m", pitLaneOffsetM));
    pitLaneWidthM =
        std::max(0.0F, config.getFloat("geometry.pit_lane_width_m", pitLaneWidthM));
    pitLaneStartM =
        std::max(0.0F, config.getFloat("geometry.pit_lane_start_m", pitLaneStartM));
    pitLaneEndM =
        std::max(pitLaneStartM, config.getFloat("geometry.pit_lane_end_m", pitLaneEndM));
    pitBoxCount =
        std::clamp(config.getInt("geometry.pit_box_count", pitBoxCount), 1, 80);
    pitBoxDepthM =
        std::max(0.0F, config.getFloat("geometry.pit_box_depth_m", pitBoxDepthM));
    pitWallHeightM =
        std::max(0.2F, config.getFloat("geometry.pit_wall_height_m", pitWallHeightM));
    yardOfBricksWidthM =
        std::max(0.0F, config.getFloat("geometry.yard_of_bricks_width_m", yardOfBricksWidthM));
    yardOfBricksPositionM =
        std::max(0.0F, config.getFloat("geometry.yard_of_bricks_position_m", yardOfBricksPositionM));
    spawnDistanceBeforeStartM = std::max(
        1.0F, config.getFloat("timing.spawn_distance_before_start_m", spawnDistanceBeforeStartM));
    spawnDistanceM = std::max(0.0F, config.getFloat("spawn_distance_m", spawnDistanceM));
    spawnHeadingOffsetRadians =
        config.getFloat("spawn_heading_offset_deg", spawnHeadingOffsetRadians / kDegreesToRadians) *
        kDegreesToRadians;
    asphaltGripMultiplier =
        std::max(
            0.05F,
            config.getFloat(
                "surfaces.asphalt_grip_multiplier",
                config.getFloat("surface_asphalt.lateral_grip", asphaltGripMultiplier)));
    apronGripMultiplier =
        std::max(0.05F, config.getFloat("surfaces.apron_grip_multiplier", apronGripMultiplier));
    curbGripMultiplier =
        std::max(0.05F, config.getFloat("surface_curb.lateral_grip", curbGripMultiplier));
    curbLongitudinalGripMultiplier =
        std::max(0.05F, config.getFloat("surface_curb.longitudinal_grip", curbLongitudinalGripMultiplier));
    curbResistanceMultiplier =
        std::max(0.1F, config.getFloat("surface_curb.rolling_resistance", curbResistanceMultiplier));
    grassGripMultiplier =
        std::max(
            0.05F,
            config.getFloat(
                "surfaces.grass_grip_multiplier",
                config.getFloat("surface_grass.lateral_grip", grassGripMultiplier)));
    grassLongitudinalGripMultiplier = std::max(
        grassGripMultiplier,
        config.getFloat(
            "surfaces.grass_longitudinal_grip_multiplier",
            config.getFloat("surface_grass.longitudinal_grip", grassLongitudinalGripMultiplier)));
    asphaltResistanceMultiplier = std::max(
        0.1F,
        config.getFloat(
            "surfaces.asphalt_resistance_multiplier",
            config.getFloat("surface_asphalt.rolling_resistance", asphaltResistanceMultiplier)));
    apronResistanceMultiplier = std::max(
        0.1F, config.getFloat("surfaces.apron_resistance_multiplier", apronResistanceMultiplier));
    grassResistanceMultiplier = std::max(
        0.1F,
        config.getFloat(
            "surfaces.grass_resistance_multiplier",
            config.getFloat("surface_grass.rolling_resistance", grassResistanceMultiplier)));
    terrainHeightEnabled = config.getBool(
        "terrain.terrain_height_enabled",
        config.getBool("terrain_height_enabled", terrainHeightEnabled));
    asphaltRoughnessM = std::clamp(
        config.getFloat("terrain.asphalt_roughness_m", config.getFloat("asphalt_roughness_m", asphaltRoughnessM)),
        0.0F,
        0.025F);
    ovalSeamAmpM = std::clamp(
        config.getFloat("terrain.oval_seam_amp_m", config.getFloat("oval_seam_amp_m", ovalSeamAmpM)),
        0.0F,
        0.040F);
    ovalUndulationAmpM = std::clamp(
        config.getFloat(
            "terrain.oval_undulation_amp_m",
            config.getFloat("oval_undulation_amp_m", ovalUndulationAmpM)),
        0.0F,
        0.040F);
    curbHeightM = std::clamp(
        config.getFloat("terrain.curb_height_m", config.getFloat("curb_height_m", curbHeightM)),
        0.0F,
        0.090F);
    contactHeightFilterTauS = std::clamp(
        config.getFloat(
            "terrain.contact_height_filter_tau_s",
            config.getFloat("contact_height_filter_tau_s", contactHeightFilterTauS)),
        0.0F,
        0.20F);
    wallRestitution =
        std::clamp(config.getFloat("barrier.restitution", config.getFloat("wall_restitution", wallRestitution)), 0.0F, 0.8F);
    wallFriction = std::clamp(config.getFloat("wall_friction", wallFriction), 0.0F, 1.0F);
    renderSegments = std::clamp(config.getInt("render.segments", renderSegments), 64, 2048);
    if (straightRoadWidthM <= 0.0F) {
        straightRoadWidthM = roadWidthM;
    }
    if (turnRoadWidthM <= 0.0F) {
        turnRoadWidthM = roadWidthM;
    }
    checkpointsM = parseFloatArray(config.getString("checkpoints_m", config.getString("checkpoints", "")));
    elevationSpine = parseFloatPairs(config.getString("elevation_spine", ""));
    if (elevationSpine.empty() && (type == "hillcircuit" || type == "hill_circuit")) {
        elevationSpine = defaultHillElevationSpine();
    }
}

OvalTrack::OvalTrack(TrackConfig config) : config_(std::move(config)) {
    halfStraightLengthM_ = config_.straightLengthM * 0.5F;
    lapLengthM_ = 2.0F * config_.straightLengthM +
                  2.0F * config_.shortChuteLengthM +
                  kTwoPi * config_.cornerRadiusM;
    if (config_.checkpointsM.empty()) {
        config_.checkpointsM = {0.0F, lapLengthM_ * 0.25F, lapLengthM_ * 0.5F, lapLengthM_ * 0.75F};
    }
    std::sort(config_.checkpointsM.begin(), config_.checkpointsM.end());
}

TrackPose OvalTrack::poseAtDistance(float distanceM) const {
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

TrackPose OvalTrack::spawnPose() const {
    const float startFinishDistanceM =
        config_.checkpointsM.empty() ? config_.yardOfBricksPositionM : config_.checkpointsM.front();
    return poseAtDistance(startFinishDistanceM - config_.spawnDistanceBeforeStartM);
}

TrackSample OvalTrack::sample(float positionX, float positionZ) const {
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
    float lateralSlope = std::tan(pose.bankRadians);
    if (sample.lateralOffsetM < -roadHalfWidth) {
        heightOffset = 0.0F;
        lateralSlope = 0.0F;
    } else {
        heightOffset = std::clamp(heightOffset, 0.0F, outerWallOffsetM() + roadHalfWidth);
    }
    sample.heightM = heightOffset * std::tan(pose.bankRadians);
    sample.smoothHeightM = sample.heightM;
    sample.contactHeightFilterTauS = config_.contactHeightFilterTauS;
    sample.terrainHeightEnabled = config_.terrainHeightEnabled;
    assignRoadNormal(sample, 0.0F, lateralSlope);

    if (config_.terrainHeightEnabled) {
        float proceduralHeight = 0.0F;
        float roughnessMagnitude = 0.0F;
        if (sample.surface == TrackSurface::Asphalt) {
            proceduralHeight += deterministicAsphaltRoughness(
                sample.distanceM,
                sample.lateralOffsetM,
                config_.asphaltRoughnessM);
            proceduralHeight += ovalUndulation(
                sample.distanceM,
                sample.lateralOffsetM,
                config_.ovalUndulationAmpM);
            roughnessMagnitude = config_.asphaltRoughnessM;
        } else if (sample.surface == TrackSurface::Apron) {
            proceduralHeight += deterministicAsphaltRoughness(
                sample.distanceM + 11.0F,
                sample.lateralOffsetM,
                config_.asphaltRoughnessM * 0.45F);
            roughnessMagnitude = config_.asphaltRoughnessM * 0.45F;
        }

        const float innerEdgeDistance = std::abs(sample.lateralOffsetM + roadHalfWidth);
        const float outerEdgeDistance = std::abs(sample.lateralOffsetM - roadHalfWidth);
        const float seam =
            config_.ovalSeamAmpM *
            std::max(
                gaussianBump(innerEdgeDistance, 0.55F),
                0.35F * gaussianBump(outerEdgeDistance, 0.65F));
        proceduralHeight += seam;
        roughnessMagnitude = std::max(roughnessMagnitude, seam);

        proceduralHeight = std::clamp(proceduralHeight, -0.030F, 0.030F);
        sample.heightM += proceduralHeight;
        sample.surfaceRoughnessM = std::clamp(roughnessMagnitude, 0.0F, 0.040F);
    }
    return sample;
}

float OvalTrack::bankForDistance(float distanceM) const {
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

float OvalTrack::normalizeDistance(float distanceM) const {
    float normalized = std::fmod(distanceM, lapLengthM_);
    if (normalized < 0.0F) {
        normalized += lapLengthM_;
    }
    return normalized;
}

CheckpointState OvalTrack::checkpointState(float distanceM) const {
    const float normalized = normalizeDistance(distanceM);
    const float startFinishDistanceM =
        config_.checkpointsM.empty() ? normalizeDistance(config_.yardOfBricksPositionM)
                                     : normalizeDistance(config_.checkpointsM.front());
    const float relativeDistance = normalizeDistance(normalized - startFinishDistanceM);
    int sector = 0;
    for (std::size_t checkpointIndex = 1U;
         checkpointIndex < config_.checkpointsM.size();
         ++checkpointIndex) {
        const float relativeCheckpoint =
            normalizeDistance(config_.checkpointsM[checkpointIndex] - startFinishDistanceM);
        if (relativeDistance + 0.001F >= relativeCheckpoint) {
            ++sector;
        }
    }
    CheckpointState state;
    state.distanceM = normalized;
    state.sector = std::clamp(
        sector,
        0,
        std::max(0, static_cast<int>(config_.checkpointsM.size()) - 1));
    state.checkpointCount = std::max(1, static_cast<int>(config_.checkpointsM.size()));
    state.startFinishDistanceM = startFinishDistanceM;
    return state;
}

BarrierSample OvalTrack::outerBarrier(float positionX, float positionZ) const {
    const TrackSample barrierSample = sample(positionX, positionZ);
    return {
        outerWallOffsetM() - barrierSample.lateralOffsetM,
        {barrierSample.rightX},
        {barrierSample.rightZ},
        config_.wallRestitution,
    };
}

BarrierSample OvalTrack::innerBarrier(float positionX, float positionZ) const {
    const TrackSample barrierSample = sample(positionX, positionZ);
    return {
        barrierSample.lateralOffsetM - innerBarrierOffsetM(),
        {-barrierSample.rightX},
        {-barrierSample.rightZ},
        config_.wallRestitution,
    };
}

HillCircuitTrack::HillCircuitTrack(TrackConfig config) : config_(std::move(config)) {
    config_.type = "hillcircuit";
    config_.name = config_.name.empty() ? "Hill Circuit" : config_.name;
    lapLengthM_ = config_.lapLengthOverrideM > 100.0F ? config_.lapLengthOverrideM : 3600.0F;
    config_.roadWidthM = std::max(8.0F, config_.roadWidthM);
    config_.curbWidthM = config_.curbWidthM > 0.0F ? config_.curbWidthM : 1.5F;
    config_.runoffWidthM = config_.runoffWidthM > 0.0F ? config_.runoffWidthM : 6.0F;
    config_.spawnDistanceM = config_.spawnDistanceM > 0.0F ? config_.spawnDistanceM : 3400.0F;
    if (config_.checkpointsM.empty()) {
        config_.checkpointsM = {620.0F, 1300.0F, 1960.0F, 2560.0F};
    }
    if (config_.elevationSpine.empty()) {
        config_.elevationSpine = defaultHillElevationSpine();
    }
    std::sort(config_.checkpointsM.begin(), config_.checkpointsM.end());
    std::sort(
        config_.elevationSpine.begin(),
        config_.elevationSpine.end(),
        [](const auto& lhs, const auto& rhs) { return lhs[0] < rhs[0]; });
    elevationSlopeCount_ = std::min(config_.elevationSpine.size(), elevationSlopes_.size());
    for (std::size_t index = 0; index < elevationSlopeCount_; ++index) {
        const std::size_t previous = index == 0U ? 0U : index - 1U;
        const std::size_t next = std::min(index + 1U, elevationSlopeCount_ - 1U);
        const float distanceSpan = std::max(
            0.001F,
            config_.elevationSpine[next][0] - config_.elevationSpine[previous][0]);
        elevationSlopes_[index] =
            (config_.elevationSpine[next][1] - config_.elevationSpine[previous][1]) /
            distanceSpan;
    }
    buildCenterline();
}

float HillCircuitTrack::normalizeDistance(float distanceM) const {
    float normalized = std::fmod(distanceM, lapLengthM_);
    if (normalized < 0.0F) {
        normalized += lapLengthM_;
    }
    return normalized;
}

float HillCircuitTrack::elevationAtDistance(float distanceM) const {
    if (config_.elevationSpine.empty()) {
        return 0.0F;
    }
    const float distance = normalizeDistance(distanceM);
    const auto& spine = config_.elevationSpine;
    std::size_t upper = 1U;
    while (upper < spine.size() && spine[upper][0] < distance) {
        ++upper;
    }
    if (upper >= spine.size()) {
        upper = spine.size() - 1U;
    }
    const std::size_t lower = upper > 0U ? upper - 1U : 0U;
    const float s0 = spine[lower][0];
    const float s1 = std::max(s0 + 0.001F, spine[upper][0]);
    const float y0 = spine[lower][1];
    const float y1 = spine[upper][1];
    const float m0 = lower < elevationSlopeCount_ ? elevationSlopes_[lower] : 0.0F;
    const float m1 = upper < elevationSlopeCount_ ? elevationSlopes_[upper] : 0.0F;
    const float t = std::clamp((distance - s0) / (s1 - s0), 0.0F, 1.0F);
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
    const float h10 = t3 - 2.0F * t2 + t;
    const float h01 = -2.0F * t3 + 3.0F * t2;
    const float h11 = t3 - t2;
    return h00 * y0 + h10 * (s1 - s0) * m0 + h01 * y1 + h11 * (s1 - s0) * m1;
}

float HillCircuitTrack::slopeAtDistance(float distanceM) const {
    if (elevationSlopeCount_ < 2U) {
        return 0.0F;
    }
    const float distance = normalizeDistance(distanceM);
    if (distance == cachedSlopeDistanceM_) {
        return cachedSlopeValue_;
    }

    const auto& spine = config_.elevationSpine;
    int segment = std::clamp(
        cachedSlopeSegment_,
        0,
        static_cast<int>(elevationSlopeCount_) - 2);
    const auto containsDistance = [&](int candidate) {
        const std::size_t lower = static_cast<std::size_t>(candidate);
        return distance >= spine[lower][0] && distance <= spine[lower + 1U][0];
    };
    if (!containsDistance(segment)) {
        segment = 0;
        while (segment + 1 < static_cast<int>(elevationSlopeCount_) - 1 &&
               spine[static_cast<std::size_t>(segment + 1)][0] < distance) {
            ++segment;
        }
    }

    const std::size_t lower = static_cast<std::size_t>(segment);
    const float startDistance = spine[lower][0];
    const float endDistance = std::max(startDistance + 0.001F, spine[lower + 1U][0]);
    const float alpha = std::clamp(
        (distance - startDistance) / (endDistance - startDistance),
        0.0F,
        1.0F);
    const float slope = elevationSlopes_[lower] +
                        (elevationSlopes_[lower + 1U] - elevationSlopes_[lower]) * alpha;
    cachedSlopeSegment_ = segment;
    cachedSlopeDistanceM_ = distance;
    cachedSlopeValue_ = slope;
    return slope;
}

float HillCircuitTrack::camberAtDistance(float distanceM) const {
    struct TurnCamber {
        float entryM;
        float exitM;
        int direction;
        float camberDegrees;
    };
    static constexpr TurnCamber turns[] = {
        {250.0F, 420.0F, -1, 2.0F},   {480.0F, 660.0F, -1, 1.0F},
        {780.0F, 960.0F, 1, 1.5F},    {1010.0F, 1200.0F, 1, 1.0F},
        {1260.0F, 1460.0F, -1, -1.5F}, {1510.0F, 1640.0F, -1, 4.0F},
        {1710.0F, 1800.0F, -1, 1.0F}, {1820.0F, 1920.0F, -1, 2.0F},
        {1920.0F, 1970.0F, 1, 2.0F},  {2020.0F, 2230.0F, -1, 3.0F},
        {2300.0F, 2470.0F, 1, 3.0F},  {2530.0F, 2720.0F, -1, 0.0F},
    };
    const float distance = normalizeDistance(distanceM);
    float bankDegrees = 0.0F;
    for (const TurnCamber& turn : turns) {
        if (distance < turn.entryM || distance > turn.exitM) {
            continue;
        }
        const float u = (distance - turn.entryM) / std::max(1.0F, turn.exitM - turn.entryM);
        const float blend = std::sin(kPi * std::clamp(u, 0.0F, 1.0F));
        const float signedBank = turn.camberDegrees * (turn.direction < 0 ? 1.0F : -1.0F);
        bankDegrees += signedBank * blend;
    }
    return bankDegrees * kDegreesToRadians;
}

void HillCircuitTrack::buildCenterline() {
    struct TurnSpec {
        float entryM;
        float exitM;
        int direction;
        float radiusM;
    };
    static constexpr TurnSpec turns[] = {
        {250.0F, 420.0F, -1, 160.0F}, {480.0F, 660.0F, -1, 28.0F},
        {780.0F, 960.0F, 1, 55.0F},   {1010.0F, 1200.0F, 1, 90.0F},
        {1260.0F, 1460.0F, -1, 55.0F}, {1510.0F, 1640.0F, -1, 110.0F},
        {1710.0F, 1800.0F, -1, 80.0F}, {1820.0F, 1920.0F, -1, 30.0F},
        {1920.0F, 1970.0F, 1, 25.0F}, {2020.0F, 2230.0F, -1, 130.0F},
        {2300.0F, 2470.0F, 1, 70.0F}, {2530.0F, 2720.0F, -1, 22.0F},
    };
    float rawHeadingSum = 0.0F;
    for (const TurnSpec& turn : turns) {
        rawHeadingSum += static_cast<float>(turn.direction) * (turn.exitM - turn.entryM) /
                         std::max(1.0F, turn.radiusM);
    }
    const float headingScale = -kTwoPi / std::min(-0.001F, rawHeadingSum);
    const auto curvatureAt = [&](float distance) {
        float curvature = 0.0F;
        for (const TurnSpec& turn : turns) {
            if (distance < turn.entryM || distance > turn.exitM) {
                continue;
            }
            const float length = std::max(1.0F, turn.exitM - turn.entryM);
            const float targetAngle =
                static_cast<float>(turn.direction) * length / std::max(1.0F, turn.radiusM) * headingScale;
            const float u = (distance - turn.entryM) / length;
            curvature += targetAngle * (kPi / (2.0F * length)) * std::sin(kPi * std::clamp(u, 0.0F, 1.0F));
        }
        return curvature;
    };

    const int segments = std::clamp(config_.renderSegments, 256, 420);
    centerline_.assign(static_cast<std::size_t>(segments + 1), {});
    const float ds = lapLengthM_ / static_cast<float>(segments);
    float x = 0.0F;
    float z = 0.0F;
    float heading = 0.0F;
    centerline_[0].distanceM = 0.0F;
    centerline_[0].x = x;
    centerline_[0].z = z;
    centerline_[0].heading = heading;
    for (int index = 1; index <= segments; ++index) {
        const float midDistance = (static_cast<float>(index) - 0.5F) * ds;
        heading += curvatureAt(midDistance) * ds;
        x += std::sin(heading) * ds;
        z += std::cos(heading) * ds;
        centerline_[static_cast<std::size_t>(index)].distanceM = static_cast<float>(index) * ds;
        centerline_[static_cast<std::size_t>(index)].x = x;
        centerline_[static_cast<std::size_t>(index)].z = z;
        centerline_[static_cast<std::size_t>(index)].heading = heading;
    }

    const float closeX = centerline_.back().x;
    const float closeZ = centerline_.back().z;
    for (CenterlinePoint& point : centerline_) {
        const float t = point.distanceM / lapLengthM_;
        point.x -= closeX * t;
        point.z -= closeZ * t;
    }
    centerline_.back().x = centerline_.front().x;
    centerline_.back().z = centerline_.front().z;

    for (std::size_t index = 0; index < centerline_.size(); ++index) {
        const std::size_t previous = index == 0U ? centerline_.size() - 2U : index - 1U;
        const std::size_t next = index + 1U >= centerline_.size() ? 1U : index + 1U;
        const float dx = centerline_[next].x - centerline_[previous].x;
        const float dz = centerline_[next].z - centerline_[previous].z;
        const float length = std::max(0.001F, std::hypot(dx, dz));
        CenterlinePoint& point = centerline_[index];
        point.tangentX = dx / length;
        point.tangentZ = dz / length;
        point.rightX = point.tangentZ;
        point.rightZ = -point.tangentX;
        point.heading = std::atan2(point.tangentX, point.tangentZ);
        point.elevationM = elevationAtDistance(point.distanceM);
        point.roadPitchRadians = std::atan(slopeAtDistance(point.distanceM));
        point.bankRadians = camberAtDistance(point.distanceM);
    }

    const std::size_t segmentCount = centerline_.size() - 1U;
    centerlineChunkCount_ = std::min(
        (segmentCount + kCenterlineChunkSize - 1U) / kCenterlineChunkSize,
        kMaxCenterlineChunks);
    for (std::size_t chunkIndex = 0; chunkIndex < centerlineChunkCount_; ++chunkIndex) {
        CenterlineChunkBounds& bounds = centerlineChunkBounds_[chunkIndex];
        bounds.firstSegment = chunkIndex * kCenterlineChunkSize;
        bounds.segmentCount = std::min(
            kCenterlineChunkSize,
            segmentCount - bounds.firstSegment);
        bounds.minimumX = std::numeric_limits<float>::max();
        bounds.maximumX = std::numeric_limits<float>::lowest();
        bounds.minimumZ = std::numeric_limits<float>::max();
        bounds.maximumZ = std::numeric_limits<float>::lowest();
        for (std::size_t pointIndex = bounds.firstSegment;
             pointIndex <= bounds.firstSegment + bounds.segmentCount;
             ++pointIndex) {
            bounds.minimumX = std::min(bounds.minimumX, centerline_[pointIndex].x);
            bounds.maximumX = std::max(bounds.maximumX, centerline_[pointIndex].x);
            bounds.minimumZ = std::min(bounds.minimumZ, centerline_[pointIndex].z);
            bounds.maximumZ = std::max(bounds.maximumZ, centerline_[pointIndex].z);
        }
    }
}

HillCircuitTrack::CenterlinePoint HillCircuitTrack::interpolatedCenterline(float distanceM) const {
    if (centerline_.empty()) {
        return {};
    }
    const float distance = normalizeDistance(distanceM);
    const float normalizedIndex =
        distance / lapLengthM_ * static_cast<float>(centerline_.size() - 1U);
    const std::size_t index = std::min<std::size_t>(
        static_cast<std::size_t>(std::floor(normalizedIndex)),
        centerline_.size() - 2U);
    const float t = normalizedIndex - static_cast<float>(index);
    const CenterlinePoint& a = centerline_[index];
    const CenterlinePoint& b = centerline_[index + 1U];
    CenterlinePoint result;
    result.distanceM = distance;
    result.x = a.x + (b.x - a.x) * t;
    result.z = a.z + (b.z - a.z) * t;
    result.tangentX = a.tangentX + (b.tangentX - a.tangentX) * t;
    result.tangentZ = a.tangentZ + (b.tangentZ - a.tangentZ) * t;
    const float tangentLength = std::max(0.001F, std::hypot(result.tangentX, result.tangentZ));
    result.tangentX /= tangentLength;
    result.tangentZ /= tangentLength;
    result.rightX = result.tangentZ;
    result.rightZ = -result.tangentX;
    result.heading = std::atan2(result.tangentX, result.tangentZ);
    result.elevationM = elevationAtDistance(distance);
    result.roadPitchRadians = std::atan(slopeAtDistance(distance));
    result.bankRadians = camberAtDistance(distance);
    return result;
}

TrackPose HillCircuitTrack::poseAtDistance(float distanceM) const {
    const CenterlinePoint centerline = interpolatedCenterline(distanceM);
    TrackPose pose;
    pose.centerX = centerline.x;
    pose.centerZ = centerline.z;
    pose.tangentX = centerline.tangentX;
    pose.tangentZ = centerline.tangentZ;
    pose.rightX = centerline.rightX;
    pose.rightZ = centerline.rightZ;
    pose.yawRadians = centerline.heading;
    pose.bankRadians = centerline.bankRadians;
    pose.roadPitchRadians = centerline.roadPitchRadians;
    pose.centerHeightM = centerline.elevationM;
    pose.distanceM = centerline.distanceM;
    pose.roadHalfWidthM = roadHalfWidthM();
    return pose;
}

TrackPose HillCircuitTrack::spawnPose() const {
    TrackPose pose = poseAtDistance(config_.spawnDistanceM);
    pose.yawRadians += config_.spawnHeadingOffsetRadians;
    return pose;
}

TrackSample HillCircuitTrack::nearestSample(
    float positionX,
    float positionZ,
    float distanceHintM,
    bool constrainToHint) const {
    const float normalizedHint = normalizeDistance(distanceHintM);
    const float cachedHintDelta = std::abs(cachedNearestSample_.distanceM - normalizedHint);
    const float wrappedCachedHintDelta = std::min(cachedHintDelta, lapLengthM_ - cachedHintDelta);
    const float hintedCacheToleranceM =
        centerline_.size() > 1U
            ? 4.0F * lapLengthM_ / static_cast<float>(centerline_.size() - 1U)
            : 0.0F;
    if (cachedNearestSampleValid_ && positionX == cachedNearestPositionX_ &&
        positionZ == cachedNearestPositionZ_ &&
        (!constrainToHint || wrappedCachedHintDelta <= hintedCacheToleranceM)) {
        return cachedNearestSample_;
    }

    TrackSample sample;
    if (centerline_.size() < 2U) {
        return sample;
    }
    float bestDistanceSq = std::numeric_limits<float>::max();
    float bestDistanceM = 0.0F;
    std::size_t bestSegment = 0U;
    const std::size_t segmentCount = centerline_.size() - 1U;
    const auto evaluateSegment = [&](std::size_t index) {
        const CenterlinePoint& a = centerline_[index];
        const CenterlinePoint& b = centerline_[index + 1U];
        const float abX = b.x - a.x;
        const float abZ = b.z - a.z;
        const float abLenSq = std::max(0.001F, abX * abX + abZ * abZ);
        const float t = std::clamp(((positionX - a.x) * abX + (positionZ - a.z) * abZ) / abLenSq, 0.0F, 1.0F);
        const float x = a.x + abX * t;
        const float z = a.z + abZ * t;
        const float dx = positionX - x;
        const float dz = positionZ - z;
        const float distanceSq = dx * dx + dz * dz;
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            bestDistanceM = a.distanceM + (b.distanceM - a.distanceM) * t;
            bestSegment = index;
        }
    };

    constexpr int kLocalSearchRadius = 4;
    int localSearchSegment = cachedNearestSegment_;
    if (constrainToHint) {
        localSearchSegment = static_cast<int>(
            normalizedHint / lapLengthM_ * static_cast<float>(segmentCount));
        localSearchSegment = std::clamp(
            localSearchSegment,
            0,
            static_cast<int>(segmentCount) - 1);
    }
    if (localSearchSegment >= 0) {
        for (int offset = -kLocalSearchRadius; offset <= kLocalSearchRadius; ++offset) {
            const int wrapped =
                (localSearchSegment + offset + static_cast<int>(segmentCount)) %
                static_cast<int>(segmentCount);
            evaluateSegment(static_cast<std::size_t>(wrapped));
        }
    }

    if (!constrainToHint) {
        for (std::size_t chunkIndex = 0; chunkIndex < centerlineChunkCount_; ++chunkIndex) {
            const CenterlineChunkBounds& bounds = centerlineChunkBounds_[chunkIndex];
            const float boundsDx =
                positionX < bounds.minimumX
                    ? bounds.minimumX - positionX
                    : (positionX > bounds.maximumX ? positionX - bounds.maximumX : 0.0F);
            const float boundsDz =
                positionZ < bounds.minimumZ
                    ? bounds.minimumZ - positionZ
                    : (positionZ > bounds.maximumZ ? positionZ - bounds.maximumZ : 0.0F);
            if (boundsDx * boundsDx + boundsDz * boundsDz > bestDistanceSq) {
                continue;
            }
            const std::size_t endSegment = bounds.firstSegment + bounds.segmentCount;
            for (std::size_t index = bounds.firstSegment; index < endSegment; ++index) {
                evaluateSegment(index);
            }
        }
    }
    cachedNearestSegment_ = static_cast<int>(bestSegment);

    const TrackPose pose = poseAtDistance(bestDistanceM);
    static_cast<TrackPose&>(sample) = pose;
    const float deltaX = positionX - pose.centerX;
    const float deltaZ = positionZ - pose.centerZ;
    sample.lateralOffsetM = deltaX * pose.rightX + deltaZ * pose.rightZ;
    const float absoluteOffset = std::abs(sample.lateralOffsetM);
    if (absoluteOffset <= roadHalfWidthM()) {
        sample.surface = TrackSurface::Asphalt;
        sample.gripMultiplier = config_.asphaltGripMultiplier;
        sample.longitudinalGripMultiplier = config_.asphaltGripMultiplier;
        sample.resistanceMultiplier = config_.asphaltResistanceMultiplier;
    } else if (absoluteOffset <= curbOuterOffsetM()) {
        sample.surface = TrackSurface::Curb;
        sample.gripMultiplier = config_.curbGripMultiplier;
        sample.longitudinalGripMultiplier = config_.curbLongitudinalGripMultiplier;
        sample.resistanceMultiplier = config_.curbResistanceMultiplier;
    } else {
        sample.surface = TrackSurface::Grass;
        sample.gripMultiplier = config_.grassGripMultiplier;
        sample.longitudinalGripMultiplier = config_.grassLongitudinalGripMultiplier;
        sample.resistanceMultiplier = config_.grassResistanceMultiplier;
    }
    const float longitudinalSlope = std::tan(pose.roadPitchRadians);
    float lateralSlope = std::tan(pose.bankRadians);
    sample.heightM = pose.centerHeightM + sample.lateralOffsetM * lateralSlope;
    sample.smoothHeightM = sample.heightM;
    sample.contactHeightFilterTauS = config_.contactHeightFilterTauS;
    sample.terrainHeightEnabled = config_.terrainHeightEnabled;
    assignRoadNormal(sample, longitudinalSlope, lateralSlope);

    if (config_.terrainHeightEnabled) {
        float proceduralHeight = 0.0F;
        float roughnessMagnitude = 0.0F;
        if (sample.surface == TrackSurface::Asphalt) {
            proceduralHeight += deterministicAsphaltRoughness(
                sample.distanceM,
                sample.lateralOffsetM,
                config_.asphaltRoughnessM * 0.75F);
            roughnessMagnitude = config_.asphaltRoughnessM * 0.75F;
        } else if (sample.surface == TrackSurface::Curb) {
            const float curbDistance = std::clamp(
                absoluteOffset - roadHalfWidthM(),
                0.0F,
                std::max(0.001F, config_.curbWidthM));
            const float curbU = curbDistance / std::max(0.001F, config_.curbWidthM);
            const float edgeRamp = smoothStep5(curbU / 0.22F);
            const float outerFeather = 1.0F - 0.18F * smoothStep5((curbU - 0.78F) / 0.22F);
            const float phase = fract(sample.distanceM / 0.50F);
            const float rumble = 0.72F + 0.28F * triangle01(phase);
            const float curbHeight = config_.curbHeightM * edgeRamp * outerFeather * rumble;
            proceduralHeight += curbHeight;
            sample.curbHeightM = curbHeight;
            sample.curbPhase = phase;
            roughnessMagnitude = std::max(config_.asphaltRoughnessM, curbHeight * 0.35F);
            const float curbSlopeSign = sample.lateralOffsetM >= 0.0F ? 1.0F : -1.0F;
            lateralSlope += curbSlopeSign * std::clamp(
                config_.curbHeightM / std::max(0.25F, config_.curbWidthM) * 0.32F,
                0.0F,
                0.06F);
            assignRoadNormal(sample, longitudinalSlope, lateralSlope);
        } else if (sample.surface == TrackSurface::Grass) {
            proceduralHeight += deterministicAsphaltRoughness(
                sample.distanceM + 29.0F,
                sample.lateralOffsetM,
                config_.asphaltRoughnessM * 0.35F);
            roughnessMagnitude = config_.asphaltRoughnessM * 0.35F;
        }
        sample.heightM += std::clamp(proceduralHeight, -0.018F, 0.075F);
        sample.surfaceRoughnessM = std::clamp(roughnessMagnitude, 0.0F, 0.080F);
    }
    cachedNearestPositionX_ = positionX;
    cachedNearestPositionZ_ = positionZ;
    cachedNearestSample_ = sample;
    cachedNearestSampleValid_ = true;
    return sample;
}

TrackSample HillCircuitTrack::sample(float positionX, float positionZ) const {
    return nearestSample(positionX, positionZ, 0.0F, false);
}

TrackSample HillCircuitTrack::sampleNearDistance(
    float positionX,
    float positionZ,
    float distanceHintM) const {
    return nearestSample(positionX, positionZ, distanceHintM, true);
}

CheckpointState HillCircuitTrack::checkpointState(float distanceM) const {
    const float normalized = normalizeDistance(distanceM);
    int sector = 0;
    for (const float checkpoint : config_.checkpointsM) {
        if (normalized >= checkpoint) {
            ++sector;
        }
    }
    return {
        normalized,
        sector,
        static_cast<int>(config_.checkpointsM.size()),
    };
}

BarrierSample HillCircuitTrack::outerBarrier(float positionX, float positionZ) const {
    const TrackSample barrierSample = sample(positionX, positionZ);
    const float limit = barrierOffsetM();
    return {
        limit - barrierSample.lateralOffsetM,
        {barrierSample.rightX},
        {barrierSample.rightZ},
        config_.wallRestitution,
    };
}

BarrierSample HillCircuitTrack::innerBarrier(float positionX, float positionZ) const {
    const TrackSample barrierSample = sample(positionX, positionZ);
    const float limit = barrierOffsetM();
    return {
        barrierSample.lateralOffsetM + limit,
        {-barrierSample.rightX},
        {-barrierSample.rightZ},
        config_.wallRestitution,
    };
}

std::unique_ptr<Track> makeTrack(TrackConfig config) {
    if (config.type == "hillcircuit" || config.type == "hill_circuit") {
        return std::make_unique<HillCircuitTrack>(std::move(config));
    }
    if (config.type != "oval" && config.type != "speedway") {
        config.type = "oval";
    }
    return std::make_unique<OvalTrack>(std::move(config));
}

}  // namespace sim
