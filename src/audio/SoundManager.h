#pragma once

#include <atomic>
#include <string>

#include <SDL3/SDL.h>

#include "game/RaceSession.h"
#include "input/InputActions.h"
#include "physics/Vehicle.h"

namespace sim {

class SoundManager {
public:
    bool initialize();
    void shutdown();
    void update(const VehicleState& vehicle, const InputActions& input, const RaceTelemetry& race);

    [[nodiscard]] bool enabled() const { return stream_ != nullptr; }
    [[nodiscard]] const std::string& error() const { return error_; }

private:
    static void SDLCALL audioCallback(
        void* userdata,
        SDL_AudioStream* stream,
        int additionalAmount,
        int totalAmount);
    void renderSamples(float* samples, int sampleCount);

    SDL_AudioStream* stream_ = nullptr;
    std::string error_;
    std::atomic<float> rpm_{1200.0F};
    std::atomic<float> throttle_{0.0F};
    std::atomic<float> tireScrub_{0.0F};
    std::atomic<float> impactTrigger_{0.0F};
    std::atomic<float> burbleTrigger_{0.0F};
    std::atomic<float> offThrottleBurble_{0.0F};
    float enginePhase_ = 0.0F;
    float crankPhase_ = 0.0F;
    float turboPhase_ = 0.0F;
    float engineFilter_ = 0.0F;
    float exhaustFilter_ = 0.0F;
    float mechanicalFilter_ = 0.0F;
    float rpmSmoothed_ = 3000.0F;
    float throttleSmoothed_ = 0.0F;
    float firingJitter_ = 0.0F;
    float burbleEnvelope_ = 0.0F;
    float burbleTonePhase_ = 0.0F;
    float burbleCooldownSeconds_ = 0.0F;
    float previousThrottle_ = 0.0F;
    float squealPhase_ = 0.0F;
    float noiseFilter_ = 0.0F;
    float impactPhase_ = 0.0F;
    float impactEnvelope_ = 0.0F;
    unsigned int noiseState_ = 0x12345678U;
    int sampleRate_ = 48000;
    bool previousWallContact_ = false;
};

}  // namespace sim
