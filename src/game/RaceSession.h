#pragma once

#include <array>
#include <memory>

#include "game/LapTimer.h"
#include "game/Track.h"

namespace sim {

struct InputActions;
class Vehicle;
struct VehicleState;

struct GhostPose {
    bool available = false;
    float positionX = 0.0F;
    float positionZ = 0.0F;
    float yawRadians = 0.0F;
    float heightM = 0.0F;
    float bankRadians = 0.0F;
};

struct RaceTelemetry {
    LapState lap;
    TrackSurface surface = TrackSurface::Asphalt;
    float lateralOffsetM = 0.0F;
    float progressM = 0.0F;
    float lapLengthM = 1.0F;
    float gripMultiplier = 1.0F;
    float longitudinalGripMultiplier = 1.0F;
    float bankRadians = 0.0F;
    float vehicleHeightM = 0.0F;
    bool wallContact = false;
    GhostPose ghost;
};

class RaceSession {
public:
    explicit RaceSession(TrackConfig config);

    void resetVehicle(Vehicle& vehicle);
    void resetRecords();
    void beginRenderFrame();
    void step(Vehicle& vehicle, const InputActions& input, float deltaSeconds);

    [[nodiscard]] const Track& track() const { return *track_; }
    [[nodiscard]] const RaceTelemetry& telemetry() const { return telemetry_; }

private:
    struct GhostSample {
        float lapSeconds = 0.0F;
        float positionX = 0.0F;
        float positionZ = 0.0F;
        float yawRadians = 0.0F;
    };

    void refreshTelemetry(const TrackSample& sample, bool wallContact);
    void appendGhostSample(const VehicleState& vehicle, float lapSeconds, bool force);
    void saveBestGhost(float lapSeconds);
    void updateGhostPose();

    static constexpr int kMaxGhostSamples = 2048;
    std::unique_ptr<Track> track_;
    LapTimer lapTimer_;
    RaceTelemetry telemetry_;
    std::array<GhostSample, kMaxGhostSamples> currentGhost_{};
    std::array<GhostSample, kMaxGhostSamples> bestGhost_{};
    int currentGhostCount_ = 0;
    int bestGhostCount_ = 0;
    float bestGhostLapSeconds_ = 0.0F;
    unsigned int renderFrameIndex_ = 0;
    unsigned int lastGhostSampleFrame_ = 0;
    bool currentGhostValid_ = true;
};

}  // namespace sim
