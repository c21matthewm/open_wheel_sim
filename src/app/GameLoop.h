#pragma once

#include <chrono>

namespace sim {

class GameLoop {
public:
    GameLoop(float fixedDeltaSeconds, float maxFrameDeltaSeconds);

    float beginFrame();
    [[nodiscard]] bool shouldStep() const { return accumulator_ >= fixedDeltaSeconds_; }
    void consumeStep() { accumulator_ -= fixedDeltaSeconds_; }
    [[nodiscard]] float fixedDeltaSeconds() const { return fixedDeltaSeconds_; }
    [[nodiscard]] float interpolationAlpha() const { return accumulator_ / fixedDeltaSeconds_; }

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point previousTime_;
    float fixedDeltaSeconds_ = 1.0F / 360.0F;
    float maxFrameDeltaSeconds_ = 0.1F;
    float accumulator_ = 0.0F;
};

}  // namespace sim
