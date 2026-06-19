#include "audio/SoundManager.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace sim {
namespace {

constexpr float kTwoPi = 6.2831853071795864769F;

float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

float smoothStep(float edge0, float edge1, float value) {
    const float t = std::clamp((value - edge0) / std::max(0.0001F, edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

float randomSigned(unsigned int& state) {
    state = state * 1664525U + 1013904223U;
    return (static_cast<float>((state >> 9U) & 0x7FFFFFU) / 4194304.0F) - 1.0F;
}

}  // namespace

bool SoundManager::initialize() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        error_ = SDL_GetError();
        return false;
    }

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = sampleRate_;
    stream_ = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        &SoundManager::audioCallback,
        this);
    if (stream_ == nullptr) {
        error_ = SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        error_ = SDL_GetError();
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    return true;
}

void SoundManager::shutdown() {
    if (stream_ != nullptr) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SoundManager::update(
    const VehicleState& vehicle,
    const InputActions& input,
    const RaceTelemetry& race) {
    rpm_.store(std::clamp(vehicle.rpm, 800.0F, 11200.0F), std::memory_order_relaxed);
    const float throttle = clamp01(input.throttle);
    throttle_.store(throttle, std::memory_order_relaxed);

    const float rpmFactor = smoothStep(3600.0F, 9000.0F, vehicle.rpm);
    const float liftAmount = std::max(0.0F, previousThrottle_ - throttle);
    if (liftAmount > 0.22F && vehicle.rpm > 4200.0F) {
        const float trigger = std::clamp((liftAmount - 0.18F) * 1.65F + rpmFactor * 0.42F, 0.0F, 1.0F);
        const float existing = burbleTrigger_.load(std::memory_order_relaxed);
        burbleTrigger_.store(std::max(existing, trigger), std::memory_order_relaxed);
    }
    const float engineBraking =
        std::clamp((0.10F - throttle) / 0.10F, 0.0F, 1.0F) *
        rpmFactor *
        std::clamp((-vehicle.longitudinalG + 0.20F) / 0.90F, 0.30F, 1.0F);
    offThrottleBurble_.store(engineBraking, std::memory_order_relaxed);
    previousThrottle_ = throttle;

    const float tireUsage = std::max(
        std::max(vehicle.frontLeftTireUsage, vehicle.frontRightTireUsage),
        std::max(vehicle.rearLeftTireUsage, vehicle.rearRightTireUsage));
    const float slipAngle = std::max(
        std::abs(vehicle.frontSlipAngleRadians),
        std::abs(vehicle.rearSlipAngleRadians));
    float scrub =
        std::clamp((tireUsage - 0.65F) * 2.2F + (slipAngle - 0.045F) * 4.2F, 0.0F, 1.0F);
    if (race.surface == TrackSurface::Grass) {
        scrub = std::max(scrub, std::clamp(vehicle.speedMps / 42.0F, 0.0F, 1.0F) * 0.72F);
    }
    tireScrub_.store(scrub, std::memory_order_relaxed);

    if (race.wallContact && !previousWallContact_) {
        impactTrigger_.store(1.0F, std::memory_order_relaxed);
    }
    previousWallContact_ = race.wallContact;
}

void SDLCALL SoundManager::audioCallback(
    void* userdata,
    SDL_AudioStream* stream,
    int additionalAmount,
    int /*totalAmount*/) {
    auto* manager = static_cast<SoundManager*>(userdata);
    if (manager == nullptr || additionalAmount <= 0) {
        return;
    }

    int remainingSamples = additionalAmount / static_cast<int>(sizeof(float));
    std::array<float, 512> buffer{};
    while (remainingSamples > 0) {
        const int chunk = std::min(remainingSamples, static_cast<int>(buffer.size()));
        manager->renderSamples(buffer.data(), chunk);
        SDL_PutAudioStreamData(stream, buffer.data(), chunk * static_cast<int>(sizeof(float)));
        remainingSamples -= chunk;
    }
}

void SoundManager::renderSamples(float* samples, int sampleCount) {
    const float sampleRate = static_cast<float>(std::max(1, sampleRate_));
    const float targetRpm = rpm_.load(std::memory_order_relaxed);
    const float targetThrottle = throttle_.load(std::memory_order_relaxed);
    const float offThrottleBurble = offThrottleBurble_.load(std::memory_order_relaxed);
    const float scrub = tireScrub_.load(std::memory_order_relaxed);
    const float impact = impactTrigger_.exchange(0.0F, std::memory_order_relaxed);
    const float burbleTrigger = burbleTrigger_.exchange(0.0F, std::memory_order_relaxed);
    impactEnvelope_ = std::max(impactEnvelope_, impact);
    burbleEnvelope_ = std::max(burbleEnvelope_, burbleTrigger);

    const float scrubVolume = scrub * 0.115F;
    const float dt = 1.0F / sampleRate;

    for (int index = 0; index < sampleCount; ++index) {
        rpmSmoothed_ += (targetRpm - rpmSmoothed_) * (1.0F - std::exp(-dt * 34.0F));
        throttleSmoothed_ += (targetThrottle - throttleSmoothed_) * (1.0F - std::exp(-dt * 46.0F));
        const float rpmNorm = std::clamp((rpmSmoothed_ - 2400.0F) / 8800.0F, 0.0F, 1.0F);
        const float crankFrequency = std::clamp(rpmSmoothed_ / 60.0F, 35.0F, 190.0F);
        const float firingFrequency = crankFrequency * 3.0F;  // Four-stroke V6: three firing events per crank rev.

        crankPhase_ += crankFrequency / sampleRate;
        crankPhase_ -= std::floor(crankPhase_);
        turboPhase_ += (930.0F + rpmNorm * 5200.0F + throttleSmoothed_ * 900.0F) / sampleRate;
        turboPhase_ -= std::floor(turboPhase_);

        enginePhase_ += firingFrequency / sampleRate;
        if (enginePhase_ >= 1.0F) {
            enginePhase_ -= std::floor(enginePhase_);
            firingJitter_ = randomSigned(noiseState_) * (0.014F + throttleSmoothed_ * 0.020F);
        }

        const float phase = enginePhase_;
        const float firingAngle = phase * kTwoPi;
        const float crankAngle = crankPhase_ * kTwoPi;
        const float pressureSpike = std::exp(-phase * (22.0F - throttleSmoothed_ * 5.0F));
        const float exhaustTail = std::exp(-phase * 4.3F);
        const float combustionPulse =
            pressureSpike * (1.15F + throttleSmoothed_ * 0.85F) -
            exhaustTail * (0.20F + throttleSmoothed_ * 0.08F);
        const float harmonicOrders =
            std::sin(firingAngle + firingJitter_) * 0.55F +
            std::sin(firingAngle * 2.0F + 0.22F) * 0.30F +
            std::sin(firingAngle * 3.0F + crankAngle * 0.08F) * 0.17F +
            std::sin(firingAngle * 5.0F + 0.70F) * 0.080F +
            std::sin(crankAngle * 6.0F + 0.35F) * (0.035F + rpmNorm * 0.035F);

        const float rawNoise = randomSigned(noiseState_);
        noiseFilter_ += (rawNoise - noiseFilter_) * 0.045F;
        mechanicalFilter_ += (rawNoise - mechanicalFilter_) * 0.23F;
        const float highNoise = rawNoise - noiseFilter_;
        const float mechanicalNoise =
            (rawNoise - mechanicalFilter_) * (0.032F + rpmNorm * 0.085F) +
            highNoise * throttleSmoothed_ * rpmNorm * 0.035F;
        const float turboWhistle =
            std::sin(turboPhase_ * kTwoPi) * throttleSmoothed_ * rpmNorm * 0.026F +
            highNoise * throttleSmoothed_ * rpmNorm * 0.032F;

        const float engineRaw =
            combustionPulse * 0.58F +
            harmonicOrders * (0.62F + throttleSmoothed_ * 0.28F) +
            mechanicalNoise +
            turboWhistle;
        exhaustFilter_ += (engineRaw - exhaustFilter_) * (0.10F + throttleSmoothed_ * 0.10F + rpmNorm * 0.035F);
        engineFilter_ += (engineRaw - engineFilter_) * (0.018F + throttleSmoothed_ * 0.018F + rpmNorm * 0.018F);
        const float rasp = engineRaw - engineFilter_;
        const float overdriveInput =
            engineFilter_ * 0.72F +
            exhaustFilter_ * 0.42F +
            rasp * (0.28F + rpmNorm * 0.20F);
        const float drivenEngine =
            std::tanh(overdriveInput * (1.55F + throttleSmoothed_ * 2.35F + rpmNorm * 0.55F));
        const float engineVolume = 0.034F + throttleSmoothed_ * 0.145F + rpmNorm * 0.034F;

        burbleCooldownSeconds_ = std::max(0.0F, burbleCooldownSeconds_ - dt);
        if (offThrottleBurble > 0.04F && burbleCooldownSeconds_ <= 0.0F) {
            const float chance = randomSigned(noiseState_) * 0.5F + 0.5F;
            if (chance < 0.012F + offThrottleBurble * 0.090F) {
                burbleEnvelope_ = std::max(burbleEnvelope_, 0.18F + offThrottleBurble * 0.78F);
                burbleCooldownSeconds_ = 0.022F + (randomSigned(noiseState_) * 0.5F + 0.5F) * 0.105F;
            }
        }
        burbleTonePhase_ += (68.0F + rpmNorm * 44.0F) / sampleRate;
        burbleTonePhase_ -= std::floor(burbleTonePhase_);
        const float popNoise = highNoise * 0.72F + std::sin(burbleTonePhase_ * kTwoPi) * 0.58F;
        const float burble = std::tanh(popNoise * 2.7F) * burbleEnvelope_;
        burbleEnvelope_ *= std::exp(-dt * (38.0F + offThrottleBurble * 18.0F));
        if (burbleEnvelope_ < 0.0004F) {
            burbleEnvelope_ = 0.0F;
        }

        squealPhase_ += (880.0F + scrub * 1240.0F) / sampleRate;
        squealPhase_ -= std::floor(squealPhase_);
        const float squealTone = std::sin(squealPhase_ * kTwoPi);
        const float tire = highNoise * 0.78F + squealTone * 0.22F;

        impactPhase_ += 58.0F / sampleRate;
        impactPhase_ -= std::floor(impactPhase_);
        const float thud = std::sin(impactPhase_ * kTwoPi) * impactEnvelope_;
        impactEnvelope_ *= 0.9960F;
        if (impactEnvelope_ < 0.0005F) {
            impactEnvelope_ = 0.0F;
        }

        const float mixed =
            drivenEngine * engineVolume +
            burble * 0.105F +
            tire * scrubVolume +
            thud * 0.28F;
        samples[index] = std::clamp(mixed, -0.65F, 0.65F);
    }
}

}  // namespace sim
