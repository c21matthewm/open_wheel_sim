#include "game/LapTimer.h"

#include <algorithm>
#include <cmath>

namespace sim {

LapTimer::LapTimer(float lapLengthM) : lapLengthM_(std::max(1.0F, lapLengthM)) {}

void LapTimer::reset(float progressM) {
    reset(CheckpointState{
        progressM,
        std::clamp(static_cast<int>(progressM / std::max(1.0F, lapLengthM_ * 0.25F)), 0, 3),
        4,
    });
}

void LapTimer::reset(const CheckpointState& checkpoint) {
    state_ = {};
    state_.currentLapValid = true;
    previousProgressM_ = checkpoint.distanceM;
    startFinishDistanceM_ = std::fmod(checkpoint.startFinishDistanceM, lapLengthM_);
    if (startFinishDistanceM_ < 0.0F) {
        startFinishDistanceM_ += lapLengthM_;
    }
    initialized_ = true;
}

void LapTimer::resetRecords() {
    state_.lastLapSeconds = 0.0F;
    state_.bestLapSeconds = 0.0F;
    state_.lastLapValid = false;
}

void LapTimer::update(float progressM, bool onRacingSurface, float deltaSeconds) {
    update(
        CheckpointState{
            progressM,
            std::clamp(static_cast<int>(progressM / std::max(1.0F, lapLengthM_ * 0.25F)), 0, 3),
            4,
        },
        onRacingSurface,
        deltaSeconds);
}

void LapTimer::update(const CheckpointState& checkpoint, bool onRacingSurface, float deltaSeconds) {
    if (!initialized_) {
        reset(checkpoint);
        return;
    }

    const float progressM = checkpoint.distanceM;
    const float delta = forwardDelta(previousProgressM_, progressM);
    if (state_.active) {
        state_.currentLapSeconds += std::max(0.0F, deltaSeconds);
        state_.currentLapValid = state_.currentLapValid && onRacingSurface;
    }

    if (delta > 0.0F) {
        if (!state_.active) {
            if (crossed(previousProgressM_, delta, startFinishDistanceM_)) {
                startLap(onRacingSurface);
            }
        } else {
            while (state_.nextCheckpoint >= 1 &&
                   state_.nextCheckpoint < std::max(1, checkpoint.checkpointCount) &&
                   checkpoint.sector >= state_.nextCheckpoint) {
                ++state_.nextCheckpoint;
            }

            if (crossed(previousProgressM_, delta, startFinishDistanceM_)) {
                if (state_.nextCheckpoint >= std::max(1, checkpoint.checkpointCount)) {
                    completeLap(onRacingSurface);
                } else {
                    startLap(onRacingSurface);
                }
            }
        }
    }

    previousProgressM_ = progressM;
}

float LapTimer::forwardDelta(float fromM, float toM) const {
    float delta = toM - fromM;
    if (delta < -lapLengthM_ * 0.5F) {
        delta += lapLengthM_;
    } else if (delta > lapLengthM_ * 0.5F) {
        delta -= lapLengthM_;
    }
    return delta;
}

bool LapTimer::crossed(float fromM, float forwardDeltaM, float targetM) const {
    float distance = targetM - fromM;
    if (distance <= 0.0F) {
        distance += lapLengthM_;
    }
    return distance <= forwardDeltaM;
}

void LapTimer::startLap(bool onRacingSurface) {
    state_.active = true;
    state_.lapNumber = std::max(1, state_.completedLaps + 1);
    state_.currentLapSeconds = 0.0F;
    state_.currentLapValid = onRacingSurface;
    state_.nextCheckpoint = 1;
}

void LapTimer::completeLap(bool onRacingSurface) {
    state_.lastLapSeconds = state_.currentLapSeconds;
    state_.lastLapValid = state_.currentLapValid;
    if (state_.lastLapValid &&
        (state_.bestLapSeconds <= 0.0F || state_.lastLapSeconds < state_.bestLapSeconds)) {
        state_.bestLapSeconds = state_.lastLapSeconds;
    }
    ++state_.completedLaps;
    startLap(onRacingSurface);
}

}  // namespace sim
