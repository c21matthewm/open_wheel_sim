#include "game/LapTimer.h"

#include <algorithm>
#include <cmath>

namespace sim {

LapTimer::LapTimer(float lapLengthM) : lapLengthM_(std::max(1.0F, lapLengthM)) {}

void LapTimer::reset(float progressM) {
    state_ = {};
    state_.currentLapValid = true;
    previousProgressM_ = progressM;
    initialized_ = true;
}

void LapTimer::resetRecords() {
    state_.lastLapSeconds = 0.0F;
    state_.bestLapSeconds = 0.0F;
    state_.lastLapValid = false;
}

void LapTimer::update(float progressM, bool onRacingSurface, float deltaSeconds) {
    if (!initialized_) {
        reset(progressM);
        return;
    }

    const float delta = forwardDelta(previousProgressM_, progressM);
    if (state_.active) {
        state_.currentLapSeconds += std::max(0.0F, deltaSeconds);
        state_.currentLapValid = state_.currentLapValid && onRacingSurface;
    }

    if (delta > 0.0F) {
        if (!state_.active) {
            if (crossed(previousProgressM_, delta, 0.0F)) {
                startLap(onRacingSurface);
            }
        } else {
            const float checkpointDistance =
                static_cast<float>(state_.nextCheckpoint) * lapLengthM_ * 0.25F;
            if (state_.nextCheckpoint >= 1 && state_.nextCheckpoint <= 3 &&
                crossed(previousProgressM_, delta, checkpointDistance)) {
                ++state_.nextCheckpoint;
            }

            if (crossed(previousProgressM_, delta, 0.0F)) {
                if (state_.nextCheckpoint == 4) {
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
