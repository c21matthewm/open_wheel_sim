#pragma once

#include <string>

#include <SDL3/SDL.h>

#include "game/RaceSession.h"
#include "input/InputActions.h"
#include "input/SDLJoystickInput.h"
#include "physics/Vehicle.h"

namespace sim {

class ConfigFile;

struct ForceFeedbackConfig {
    bool enabled = true;
    float masterStrength = 0.65F;
    float centeringStrength = 0.25F;
    float damping = 0.15F;
    float aligningTorqueScale = 0.45F;
    float pneumaticTrailM = 0.055F;
    float mechanicalTrailM = 0.030F;
    float peakSlipAngleRadians = 0.118682F;
    float understeerLightening = 0.50F;
    float grassVibration = 0.18F;
    float collisionJolt = 0.75F;
    float maxForce = 1.0F;

    void load(const ConfigFile& config);
};

class ForceFeedbackManager {
public:
    ForceFeedbackManager() = default;
    ~ForceFeedbackManager();

    void initialize(const ForceFeedbackConfig& config);
    void update(
        const VehicleState& vehicle,
        const RaceTelemetry& race,
        const InputActions& input,
        const SDLJoystickInput::Device* activeDevice,
        float deltaSeconds);
    void shutdown();

    [[nodiscard]] bool active() const { return haptic_ != nullptr && effectId_ >= 0; }
    [[nodiscard]] const std::string& status() const { return status_; }
    [[nodiscard]] float outputForce() const { return smoothedForce_; }

private:
    enum class Backend {
        None,
        Constant,
        Spring,
        LeftRight,
        Rumble,
    };

    void closeDevice();
    void tryOpenDevice(const SDLJoystickInput::Device* device);
    [[nodiscard]] SDL_HapticEffect makeConstantEffect(float force) const;
    [[nodiscard]] SDL_HapticEffect makeSpringEffect(float force) const;
    [[nodiscard]] SDL_HapticEffect makeLeftRightEffect(float force) const;
    [[nodiscard]] float computeForce(
        const VehicleState& vehicle,
        const RaceTelemetry& race,
        float deltaSeconds);
    void streamForce(float force);
    [[nodiscard]] Sint16 forceToSint16(float force) const;

    ForceFeedbackConfig config_;
    SDL_Haptic* haptic_ = nullptr;
    SDL_JoystickID joystickId_ = 0;
    SDL_HapticEffectID effectId_ = -1;
    Backend backend_ = Backend::None;
    std::string status_ = "FFB: not initialized";
    float elapsedSeconds_ = 0.0F;
    float previousSteeringAngle_ = 0.0F;
    float smoothedForce_ = 0.0F;
    float collisionEnvelope_ = 0.0F;
    float collisionSign_ = 1.0F;
    bool previousWallContact_ = false;
    bool initialized_ = false;
};

}  // namespace sim
