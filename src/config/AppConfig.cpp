#include "config/AppConfig.h"

#include <algorithm>

#include "config/ConfigFile.h"

namespace sim {

void GraphicsConfig::load(const ConfigFile& config) {
    title = config.getString("window.title", title);
    width = std::max(320, config.getInt("window.width", width));
    height = std::max(240, config.getInt("window.height", height));
    fullscreen = config.getBool("window.fullscreen", fullscreen);
    borderless = config.getBool("window.borderless", borderless);
    vsync = config.getBool("render.vsync", vsync);
    renderScale = std::clamp(config.getFloat("render.scale", renderScale), 0.5F, 1.0F);
    shadowMapSize = std::clamp(config.getInt("render.shadow_map_size", shadowMapSize), 1024, 4096);
    shadowUpdateInterval =
        std::clamp(config.getInt("render.shadow_update_interval", shadowUpdateInterval), 1, 3);
    const int requestedMsaaSamples = config.getInt("render.msaa_samples", msaaSamples);
    msaaSamples = requestedMsaaSamples <= 2 ? 2 : 4;
    physicsHz = std::clamp(config.getInt("simulation.physics_hz", physicsHz), 30, 720);
    maxFrameDelta =
        std::clamp(config.getFloat("simulation.max_frame_delta", maxFrameDelta), 0.02F, 0.5F);
}

void InputConfig::load(const ConfigFile& config) {
    diagnosticsVisibleOnStart =
        config.getBool("diagnostics.visible_on_start", diagnosticsVisibleOnStart);
    keyboardSteerRate =
        std::max(0.1F, config.getFloat("keyboard.steer_rate", keyboardSteerRate));
    keyboardReturnRate =
        std::max(0.1F, config.getFloat("keyboard.return_rate", keyboardReturnRate));

    wheelEnabled = config.getBool("wheel.enabled", wheelEnabled);
    wheelNameContains = config.getString("wheel.name_contains", wheelNameContains);
    steerAxis = config.getInt("wheel.steer_axis", steerAxis);
    throttleAxis = config.getInt("wheel.throttle_axis", throttleAxis);
    brakeAxis = config.getInt("wheel.brake_axis", brakeAxis);
    shiftUpButton = config.getInt("wheel.shift_up_button", shiftUpButton);
    shiftDownButton = config.getInt("wheel.shift_down_button", shiftDownButton);
    cameraButton = config.getInt("wheel.camera_button", cameraButton);
    throttleInverted = config.getBool("wheel.throttle_inverted", throttleInverted);
    brakeInverted = config.getBool("wheel.brake_inverted", brakeInverted);
    steerDeadzone = std::clamp(config.getFloat("wheel.steer_deadzone", steerDeadzone), 0.0F, 0.5F);
    pedalDeadzone = std::clamp(config.getFloat("wheel.pedal_deadzone", pedalDeadzone), 0.0F, 0.5F);
    steerGamma = std::clamp(config.getFloat("wheel.steer_gamma", steerGamma), 0.2F, 4.0F);
    wheelSteerSensitivity =
        std::clamp(config.getFloat("wheel.steer_sensitivity", wheelSteerSensitivity), 0.1F, 5.0F);
    throttleGamma = std::clamp(config.getFloat("wheel.throttle_gamma", throttleGamma), 0.2F, 4.0F);
    brakeGamma = std::clamp(config.getFloat("wheel.brake_gamma", brakeGamma), 0.2F, 4.0F);
}

}  // namespace sim
