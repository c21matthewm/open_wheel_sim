#include "app/GameLoop.h"

#include <algorithm>

namespace sim {

GameLoop::GameLoop(float fixedDeltaSeconds, float maxFrameDeltaSeconds)
    : previousTime_(Clock::now()),
      fixedDeltaSeconds_(fixedDeltaSeconds),
      maxFrameDeltaSeconds_(maxFrameDeltaSeconds) {}

float GameLoop::beginFrame() {
    const Clock::time_point now = Clock::now();
    const float elapsed = std::chrono::duration<float>(now - previousTime_).count();
    previousTime_ = now;
    const float clamped = std::clamp(elapsed, 0.0F, maxFrameDeltaSeconds_);
    accumulator_ += clamped;
    return clamped;
}

}  // namespace sim
