#pragma once

namespace sim {

struct LapState {
    float currentLapSeconds = 0.0F;
    float lastLapSeconds = 0.0F;
    float bestLapSeconds = 0.0F;
    int lapNumber = 0;
    int completedLaps = 0;
    int nextCheckpoint = 0;
    bool active = false;
    bool currentLapValid = true;
    bool lastLapValid = false;
};

class LapTimer {
public:
    explicit LapTimer(float lapLengthM);

    void reset(float progressM);
    void resetRecords();
    void update(float progressM, bool onRacingSurface, float deltaSeconds);

    [[nodiscard]] const LapState& state() const { return state_; }

private:
    [[nodiscard]] float forwardDelta(float fromM, float toM) const;
    [[nodiscard]] bool crossed(float fromM, float forwardDeltaM, float targetM) const;
    void startLap(bool onRacingSurface);
    void completeLap(bool onRacingSurface);

    float lapLengthM_ = 1.0F;
    float previousProgressM_ = 0.0F;
    bool initialized_ = false;
    LapState state_;
};

}  // namespace sim
