#include "input/InputManager.h"

#include <algorithm>
#include <cmath>

namespace sim {

void InputManager::initialize(const InputConfig& config) {
    config_ = config;
    joysticks_.initialize();
}

void InputManager::shutdown() {
    joysticks_.shutdown();
}

void InputManager::beginFrame() {
    actions_.shiftUp = false;
    actions_.shiftDown = false;
    actions_.resetCar = false;
    actions_.toggleMenu = false;
    actions_.menuUp = false;
    actions_.menuDown = false;
    actions_.menuLeft = false;
    actions_.menuRight = false;
    actions_.menuSelect = false;
    actions_.toggleFullscreen = false;
    actions_.toggleDiagnostics = false;
    actions_.toggleOverlay = false;
    actions_.toggleCamera = false;
    actions_.quit = false;
}

void InputManager::handleEvent(const SDL_Event& event) {
    joysticks_.handleEvent(event);

    if (event.type == SDL_EVENT_QUIT) {
        actions_.quit = true;
        return;
    }
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return;
    }

    const bool commandHeld = (event.key.mod & SDL_KMOD_GUI) != 0;
    if (commandHeld && event.key.scancode == SDL_SCANCODE_Q) {
        actions_.quit = true;
    } else if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
        actions_.toggleMenu = true;
    } else if (event.key.scancode == SDL_SCANCODE_R) {
        actions_.resetCar = true;
    } else if (event.key.scancode == SDL_SCANCODE_F11 ||
               (commandHeld && event.key.scancode == SDL_SCANCODE_F)) {
        actions_.toggleFullscreen = true;
    } else if (event.key.scancode == SDL_SCANCODE_F2) {
        actions_.toggleDiagnostics = true;
    } else if (event.key.scancode == SDL_SCANCODE_F1) {
        actions_.toggleOverlay = true;
    } else if (event.key.scancode == SDL_SCANCODE_C) {
        actions_.toggleCamera = true;
    } else if (event.key.scancode == SDL_SCANCODE_E) {
        actions_.shiftUp = true;
    } else if (event.key.scancode == SDL_SCANCODE_Q) {
        actions_.shiftDown = true;
    } else if (event.key.scancode == SDL_SCANCODE_UP) {
        actions_.menuUp = true;
    } else if (event.key.scancode == SDL_SCANCODE_DOWN) {
        actions_.menuDown = true;
    } else if (event.key.scancode == SDL_SCANCODE_LEFT) {
        actions_.menuLeft = true;
    } else if (event.key.scancode == SDL_SCANCODE_RIGHT) {
        actions_.menuRight = true;
    } else if (event.key.scancode == SDL_SCANCODE_RETURN ||
               event.key.scancode == SDL_SCANCODE_KP_ENTER) {
        actions_.menuSelect = true;
    }
}

void InputManager::update(float deltaSeconds) {
    joysticks_.update();

    int keyCount = 0;
    const bool* keys = SDL_GetKeyboardState(&keyCount);
    const auto pressed = [keys, keyCount](SDL_Scancode key) {
        return static_cast<int>(key) < keyCount && keys[key];
    };

    float steeringTarget = 0.0F;
    if (pressed(SDL_SCANCODE_A) || pressed(SDL_SCANCODE_LEFT)) {
        steeringTarget -= 1.0F;
    }
    if (pressed(SDL_SCANCODE_D) || pressed(SDL_SCANCODE_RIGHT)) {
        steeringTarget += 1.0F;
    }
    const float speedFraction =
        std::clamp(std::abs(vehicleSpeedMps_) / std::max(1.0F, keyboardHighSpeedThresholdMps_), 0.0F, 1.0F);
    const float highSpeedRateScale = 1.0F + (keyboardHighSpeedScale_ - 1.0F) * speedFraction;
    const float steerRate = steeringTarget == 0.0F
                                ? config_.keyboardReturnRate
                                : config_.keyboardSteerRate * highSpeedRateScale;
    keyboardSteer_ = moveToward(keyboardSteer_, steeringTarget, steerRate * deltaSeconds);

    actions_.steer = keyboardSteer_;
    actions_.throttle = (pressed(SDL_SCANCODE_W) || pressed(SDL_SCANCODE_UP)) ? 1.0F : 0.0F;
    actions_.brake =
        (pressed(SDL_SCANCODE_S) || pressed(SDL_SCANCODE_DOWN) || pressed(SDL_SCANCODE_SPACE))
            ? 1.0F
            : 0.0F;
    actions_.wheelActive = false;

    if (!config_.wheelEnabled) {
        previousCameraButtonPressed_ = false;
        return;
    }

    const SDLJoystickInput::Device* wheel = joysticks_.firstMatching(config_.wheelNameContains);
    if (wheel == nullptr) {
        previousCameraButtonPressed_ = false;
        return;
    }

    if (config_.steerAxis >= 0 && config_.steerAxis < wheel->axisCount) {
        const float rawWheelSteer = applySignedGamma(
            applyDeadzone(wheel->axes[config_.steerAxis], config_.steerDeadzone),
            config_.steerGamma);
        actions_.steer =
            std::clamp(rawWheelSteer * config_.wheelSteerSensitivity, -1.0F, 1.0F);
    }
    if (config_.throttleAxis >= 0 && config_.throttleAxis < wheel->axisCount) {
        actions_.throttle =
            mapPedal(
                wheel->axes[config_.throttleAxis],
                config_.throttleInverted,
                config_.pedalDeadzone,
                config_.throttleGamma);
    }
    if (config_.brakeAxis >= 0 && config_.brakeAxis < wheel->axisCount) {
        actions_.brake =
            mapPedal(
                wheel->axes[config_.brakeAxis],
                config_.brakeInverted,
                config_.pedalDeadzone,
                config_.brakeGamma);
    }
    if (config_.shiftUpButton >= 0 && config_.shiftUpButton < wheel->buttonCount) {
        actions_.shiftUp = actions_.shiftUp || wheel->buttons[config_.shiftUpButton];
    }
    if (config_.shiftDownButton >= 0 && config_.shiftDownButton < wheel->buttonCount) {
        actions_.shiftDown = actions_.shiftDown || wheel->buttons[config_.shiftDownButton];
    }
    bool cameraButtonPressed = false;
    if (config_.cameraButton >= 0 && config_.cameraButton < wheel->buttonCount) {
        cameraButtonPressed = wheel->buttons[config_.cameraButton];
        if (cameraButtonPressed && !previousCameraButtonPressed_) {
            actions_.toggleCamera = true;
        }
    }
    previousCameraButtonPressed_ = cameraButtonPressed;
    actions_.wheelActive = true;
}

void InputManager::setKeyboardRates(float steerRate, float returnRate) {
    config_.keyboardSteerRate = std::max(0.1F, steerRate);
    config_.keyboardReturnRate = std::max(0.1F, returnRate);
}

void InputManager::setKeyboardHighSpeedResponse(float scale, float thresholdMps) {
    keyboardHighSpeedScale_ = std::clamp(scale, 0.05F, 1.0F);
    keyboardHighSpeedThresholdMps_ = std::max(1.0F, thresholdMps);
}

float InputManager::moveToward(float current, float target, float maxDelta) {
    if (std::abs(target - current) <= maxDelta) {
        return target;
    }
    return current + std::copysign(maxDelta, target - current);
}

float InputManager::applyDeadzone(float value, float deadzone) {
    if (std::abs(value) <= deadzone) {
        return 0.0F;
    }
    const float scaled = (std::abs(value) - deadzone) / (1.0F - deadzone);
    return std::copysign(std::clamp(scaled, 0.0F, 1.0F), value);
}

float InputManager::applySignedGamma(float value, float gamma) {
    const float magnitude = std::pow(std::abs(std::clamp(value, -1.0F, 1.0F)), std::max(0.01F, gamma));
    return std::copysign(magnitude, value);
}

float InputManager::mapPedal(float value, bool inverted, float deadzone, float gamma) {
    float mapped = inverted ? (1.0F - value) * 0.5F : (value + 1.0F) * 0.5F;
    mapped = std::clamp(mapped, 0.0F, 1.0F);
    if (mapped <= deadzone) {
        return 0.0F;
    }
    const float normalized = std::clamp((mapped - deadzone) / (1.0F - deadzone), 0.0F, 1.0F);
    return std::pow(normalized, std::max(0.01F, gamma));
}

}  // namespace sim
