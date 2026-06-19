#include "game/RaceSession.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "input/InputActions.h"
#include "physics/Vehicle.h"

namespace sim {
namespace {

constexpr float kGhostSampleIntervalSeconds = 1.0F / 30.0F;
constexpr float kGrassLowSpeedDampingPerSecond = 0.117F;
constexpr float kGrassHighSpeedDampingPerSecond = 0.325F;
constexpr float kGrassTrapResistanceScale = 1.0F;
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0F * kPi;
constexpr float kWallCollisionSlopM = 0.005F;
constexpr int kWallCollisionIterations = 4;

float angleDelta(float fromRadians, float toRadians) {
    float delta = std::fmod(toRadians - fromRadians + kPi, kTwoPi);
    if (delta < 0.0F) {
        delta += kTwoPi;
    }
    return delta - kPi;
}

int surfaceSeverity(TrackSurface surface) {
    switch (surface) {
        case TrackSurface::Grass:
            return 2;
        case TrackSurface::Apron:
            return 1;
        case TrackSurface::Curb:
            return 0;
        case TrackSurface::Asphalt:
            return 0;
    }
    return 0;
}

struct WheelSurfaceSet {
    std::array<WheelSurfaceContact, 4> contacts{};
    float grassContactFraction = 0.0F;
    TrackSurface mostSevereSurface = TrackSurface::Asphalt;
    float minimumGripMultiplier = 1.0F;
    float minimumLongitudinalGripMultiplier = 1.0F;
};

WheelSurfaceSet sampleWheelSurfaces(
    const Track& track,
    const Vehicle& vehicle,
    float distanceHintM,
    float referenceRoadHeightM) {
    WheelSurfaceSet result;
    const std::array<WheelContactPatch, 4> patches = vehicle.wheelContactPatchesWorld();
    int grassContacts = 0;
    for (std::size_t wheel = 0; wheel < patches.size(); ++wheel) {
        TrackSample sample =
            track.sampleNearDistance(patches[wheel].x, patches[wheel].z, distanceHintM);
        float resistanceMultiplier = sample.resistanceMultiplier;
        if (sample.surface == TrackSurface::Grass) {
            resistanceMultiplier *= kGrassTrapResistanceScale;
            ++grassContacts;
        }
        result.contacts[wheel] = {
            sample.gripMultiplier,
            sample.longitudinalGripMultiplier,
            resistanceMultiplier,
            sample.bankRadians,
            sample.roadPitchRadians,
            sample.tangentX,
            sample.tangentZ,
            sample.terrainHeightEnabled ? sample.heightM - referenceRoadHeightM : 0.0F,
            sample.roadNormalX,
            sample.roadNormalY,
            sample.roadNormalZ,
            sample.surfaceRoughnessM,
            sample.curbHeightM,
            sample.curbPhase,
            sample.contactHeightFilterTauS,
        };
        if (surfaceSeverity(sample.surface) > surfaceSeverity(result.mostSevereSurface)) {
            result.mostSevereSurface = sample.surface;
        }
        result.minimumGripMultiplier = std::min(result.minimumGripMultiplier, sample.gripMultiplier);
        result.minimumLongitudinalGripMultiplier =
            std::min(result.minimumLongitudinalGripMultiplier, sample.longitudinalGripMultiplier);
    }
    result.grassContactFraction = static_cast<float>(grassContacts) / static_cast<float>(patches.size());
    return result;
}

}  // namespace

RaceSession::RaceSession(TrackConfig config)
    : track_(makeTrack(std::move(config))), lapTimer_(track_->lapLengthM()) {
    telemetry_.lapLengthM = track_->lapLengthM();
}

void RaceSession::resetVehicle(Vehicle& vehicle) {
    const TrackPose spawn = track_->spawnPose();
    vehicle.reset(spawn.centerX, spawn.centerZ, spawn.yawRadians);
    const TrackSample sample = track_->sample(spawn.centerX, spawn.centerZ);
    lapTimer_.reset(track_->checkpointState(sample.distanceM));
    currentGhostCount_ = 0;
    currentGhostValid_ = true;
    lastGhostSampleFrame_ = renderFrameIndex_;
    refreshTelemetry(sample, false);
}

void RaceSession::resetRecords() {
    lapTimer_.resetRecords();
    currentGhostCount_ = 0;
    bestGhostCount_ = 0;
    bestGhostLapSeconds_ = 0.0F;
    currentGhostValid_ = true;
    lastGhostSampleFrame_ = renderFrameIndex_;
    telemetry_.ghost = {};
}

void RaceSession::beginRenderFrame() {
    ++renderFrameIndex_;
}

void RaceSession::step(Vehicle& vehicle, const InputActions& input, float deltaSeconds) {
    TrackSample sample =
        track_->sample(vehicle.current().positionX, vehicle.current().positionZ);
    WheelSurfaceSet wheelSurfaces =
        sampleWheelSurfaces(*track_, vehicle, sample.distanceM, sample.smoothHeightM);
    vehicle.step(
        input,
        deltaSeconds,
        sample.gripMultiplier,
        sample.longitudinalGripMultiplier,
        sample.resistanceMultiplier,
        sample.bankRadians,
        sample.roadPitchRadians,
        sample.tangentX,
        sample.tangentZ,
        &wheelSurfaces.contacts);

    sample = track_->sample(vehicle.current().positionX, vehicle.current().positionZ);
    bool wallContact = false;
    const float collisionRadiusM = std::hypot(
        vehicle.collisionHalfWidthM(),
        vehicle.collisionHalfLengthM());
    const BarrierSample centerOuterBarrier = track_->outerBarrier(
        vehicle.current().positionX,
        vehicle.current().positionZ);
    const BarrierSample centerInnerBarrier = track_->innerBarrier(
        vehicle.current().positionX,
        vehicle.current().positionZ);
    const bool requiresCornerCollisionTest =
        centerOuterBarrier.signedDistanceM <= collisionRadiusM + kWallCollisionSlopM ||
        centerInnerBarrier.signedDistanceM <= collisionRadiusM + kWallCollisionSlopM;
    for (int iteration = 0;
         requiresCornerCollisionTest && iteration < kWallCollisionIterations;
         ++iteration) {
        struct WallHit {
            float penetrationM = 0.0F;
            float normalX = 0.0F;
            float normalZ = 0.0F;
            float restitution = 0.0F;
            float contactOffsetX = 0.0F;
            float contactOffsetZ = 0.0F;
        };
        WallHit deepestHit;
        const VehicleState& state = vehicle.current();
        const float forwardX = std::sin(state.yawRadians);
        const float forwardZ = std::cos(state.yawRadians);
        const float rightX = forwardZ;
        const float rightZ = -forwardX;
        const float halfWidth = vehicle.collisionHalfWidthM();
        const float halfLength = vehicle.collisionHalfLengthM();

        for (const float localX : {-halfWidth, halfWidth}) {
            for (const float localZ : {-halfLength, halfLength}) {
                const float contactOffsetX = rightX * localX + forwardX * localZ;
                const float contactOffsetZ = rightZ * localX + forwardZ * localZ;
                const float sampleX = state.positionX + contactOffsetX;
                const float sampleZ = state.positionZ + contactOffsetZ;
                const BarrierSample outerBarrier = track_->outerBarrier(sampleX, sampleZ);
                const float outerPenetration = -outerBarrier.signedDistanceM;
                if (outerPenetration > deepestHit.penetrationM) {
                    deepestHit = {
                        outerPenetration,
                        outerBarrier.normalX,
                        outerBarrier.normalZ,
                        outerBarrier.restitution,
                        contactOffsetX,
                        contactOffsetZ,
                    };
                }
                const BarrierSample innerBarrier = track_->innerBarrier(sampleX, sampleZ);
                const float innerPenetration = -innerBarrier.signedDistanceM;
                if (innerPenetration > deepestHit.penetrationM) {
                    deepestHit = {
                        innerPenetration,
                        innerBarrier.normalX,
                        innerBarrier.normalZ,
                        innerBarrier.restitution,
                        contactOffsetX,
                        contactOffsetZ,
                    };
                }
            }
        }

        if (deepestHit.penetrationM <= 0.0F) {
            break;
        }
        vehicle.resolveBoundary(
            deepestHit.normalX,
            deepestHit.normalZ,
            deepestHit.penetrationM + kWallCollisionSlopM,
            deepestHit.restitution,
            deepestHit.contactOffsetX,
            deepestHit.contactOffsetZ);
        wallContact = true;
    }
    sample = track_->sample(vehicle.current().positionX, vehicle.current().positionZ);
    wheelSurfaces = sampleWheelSurfaces(*track_, vehicle, sample.distanceM, sample.smoothHeightM);

    if (wheelSurfaces.grassContactFraction > 0.0F) {
        const float speedBlend =
            std::clamp((vehicle.current().speedMps - 8.0F) / 22.0F, 0.0F, 1.0F);
        const float grassDamping =
            kGrassLowSpeedDampingPerSecond +
            (kGrassHighSpeedDampingPerSecond - kGrassLowSpeedDampingPerSecond) * speedBlend;
        vehicle.applyVelocityDamping(grassDamping * wheelSurfaces.grassContactFraction, deltaSeconds);
        sample = track_->sample(vehicle.current().positionX, vehicle.current().positionZ);
        wheelSurfaces = sampleWheelSurfaces(*track_, vehicle, sample.distanceM, sample.smoothHeightM);
    }

    const bool validRacingSurface = sample.surface != TrackSurface::Grass && !wallContact;
    const LapState beforeLap = lapTimer_.state();
    lapTimer_.update(track_->checkpointState(sample.distanceM), validRacingSurface, deltaSeconds);
    const LapState& afterLap = lapTimer_.state();

    if (!beforeLap.active && afterLap.active) {
        currentGhostCount_ = 0;
        currentGhostValid_ = afterLap.currentLapValid;
        appendGhostSample(vehicle.current(), afterLap.currentLapSeconds, true);
    } else if (beforeLap.active && afterLap.completedLaps > beforeLap.completedLaps) {
        currentGhostValid_ = currentGhostValid_ && beforeLap.currentLapValid && validRacingSurface;
        appendGhostSample(vehicle.current(), beforeLap.currentLapSeconds + deltaSeconds, true);
        const bool newBest =
            afterLap.lastLapValid && currentGhostValid_ && afterLap.bestLapSeconds > 0.0F &&
            std::abs(afterLap.bestLapSeconds - afterLap.lastLapSeconds) < 0.002F;
        if (newBest) {
            saveBestGhost(afterLap.bestLapSeconds);
        }
        currentGhostCount_ = 0;
        currentGhostValid_ = afterLap.currentLapValid;
        appendGhostSample(vehicle.current(), afterLap.currentLapSeconds, true);
    } else if (afterLap.active) {
        currentGhostValid_ = currentGhostValid_ && afterLap.currentLapValid;
        appendGhostSample(vehicle.current(), afterLap.currentLapSeconds, false);
    }

    TrackSample telemetrySample = sample;
    telemetrySample.surface = wheelSurfaces.mostSevereSurface;
    telemetrySample.gripMultiplier = wheelSurfaces.minimumGripMultiplier;
    telemetrySample.longitudinalGripMultiplier = wheelSurfaces.minimumLongitudinalGripMultiplier;
    refreshTelemetry(telemetrySample, wallContact);
}

void RaceSession::refreshTelemetry(const TrackSample& sample, bool wallContact) {
    telemetry_.lap = lapTimer_.state();
    telemetry_.surface = sample.surface;
    telemetry_.lateralOffsetM = sample.lateralOffsetM;
    telemetry_.progressM = sample.distanceM;
    telemetry_.lapLengthM = track_->lapLengthM();
    telemetry_.gripMultiplier = sample.gripMultiplier;
    telemetry_.longitudinalGripMultiplier = sample.longitudinalGripMultiplier;
    telemetry_.bankRadians = sample.bankRadians;
    telemetry_.vehicleHeightM = sample.smoothHeightM;
    telemetry_.wallContact = wallContact;
    updateGhostPose();
}

void RaceSession::appendGhostSample(const VehicleState& vehicle, float lapSeconds, bool force) {
    if (currentGhostCount_ >= kMaxGhostSamples) {
        return;
    }
    if (!force && lastGhostSampleFrame_ == renderFrameIndex_) {
        return;
    }
    if (!force && currentGhostCount_ > 0) {
        const GhostSample& previous = currentGhost_[currentGhostCount_ - 1];
        if (lapSeconds - previous.lapSeconds < kGhostSampleIntervalSeconds) {
            return;
        }
    }

    currentGhost_[currentGhostCount_++] = {
        std::max(0.0F, lapSeconds),
        vehicle.positionX,
        vehicle.positionZ,
        vehicle.yawRadians,
    };
    lastGhostSampleFrame_ = renderFrameIndex_;
}

void RaceSession::saveBestGhost(float lapSeconds) {
    bestGhostCount_ = currentGhostCount_;
    bestGhostLapSeconds_ = lapSeconds;
    std::copy_n(currentGhost_.begin(), bestGhostCount_, bestGhost_.begin());
}

void RaceSession::updateGhostPose() {
    telemetry_.ghost = {};
    const LapState& lap = lapTimer_.state();
    if (bestGhostCount_ < 2 || bestGhostLapSeconds_ <= 0.0F || !lap.active) {
        return;
    }

    const float playbackTime = std::fmod(lap.currentLapSeconds, bestGhostLapSeconds_);
    int upperIndex = 1;
    while (upperIndex < bestGhostCount_ - 1 && bestGhost_[upperIndex].lapSeconds < playbackTime) {
        ++upperIndex;
    }
    const GhostSample& previous = bestGhost_[upperIndex - 1];
    const GhostSample& next = bestGhost_[upperIndex];
    const float span = std::max(0.001F, next.lapSeconds - previous.lapSeconds);
    const float alpha = std::clamp((playbackTime - previous.lapSeconds) / span, 0.0F, 1.0F);
    const float x = previous.positionX + (next.positionX - previous.positionX) * alpha;
    const float z = previous.positionZ + (next.positionZ - previous.positionZ) * alpha;
    const float yaw = previous.yawRadians + angleDelta(previous.yawRadians, next.yawRadians) * alpha;
    const TrackSample sample = track_->sample(x, z);

    telemetry_.ghost = {
        true,
        x,
        z,
        yaw,
        sample.smoothHeightM,
        sample.bankRadians,
    };
}

}  // namespace sim
