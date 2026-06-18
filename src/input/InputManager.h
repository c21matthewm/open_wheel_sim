#pragma once

#include <SDL3/SDL.h>

#include "config/AppConfig.h"
#include "input/InputActions.h"
#include "input/SDLJoystickInput.h"

namespace sim {

class InputManager {
public:
    void initialize(const InputConfig& config);
    void shutdown();
    void beginFrame();
    void handleEvent(const SDL_Event& event);
    void update(float deltaSeconds);
    void setVehicleSpeed(float speedMps) { vehicleSpeedMps_ = speedMps; }
    void setKeyboardRates(float steerRate, float returnRate);
    void setKeyboardHighSpeedResponse(float scale, float thresholdMps);
    void centerKeyboardSteer() { keyboardSteer_ = 0.0F; }

    [[nodiscard]] const InputActions& actions() const { return actions_; }
    [[nodiscard]] const SDLJoystickInput& joysticks() const { return joysticks_; }
    [[nodiscard]] const SDLJoystickInput::Device* selectedWheel() const {
        return joysticks_.firstMatching(config_.wheelNameContains);
    }
    [[nodiscard]] float keyboardSteerRate() const { return config_.keyboardSteerRate; }
    [[nodiscard]] float keyboardReturnRate() const { return config_.keyboardReturnRate; }

private:
    static float moveToward(float current, float target, float maxDelta);
    static float applyDeadzone(float value, float deadzone);
    static float applySignedGamma(float value, float gamma);
    static float mapPedal(float value, bool inverted, float deadzone, float gamma);

    InputConfig config_;
    SDLJoystickInput joysticks_;
    InputActions actions_;
    float keyboardSteer_ = 0.0F;
    float vehicleSpeedMps_ = 0.0F;
    float keyboardHighSpeedScale_ = 0.45F;
    float keyboardHighSpeedThresholdMps_ = 90.0F;
    bool previousCameraButtonPressed_ = false;
};

}  // namespace sim
