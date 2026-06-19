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
    shadowFrustumExtentM =
        std::clamp(config.getFloat("render.shadow_frustum_extent_m", shadowFrustumExtentM), 45.0F, 180.0F);
    shadowLightDistanceM =
        std::clamp(config.getFloat("render.shadow_light_distance_m", shadowLightDistanceM), 80.0F, 280.0F);
    shadowLightHeightOffsetM = std::clamp(
        config.getFloat("render.shadow_light_height_offset_m", shadowLightHeightOffsetM),
        0.0F,
        120.0F);
    const int requestedMsaaSamples = config.getInt("render.msaa_samples", msaaSamples);
    msaaSamples = requestedMsaaSamples <= 2 ? 2 : 4;
    bloomHalfWeight =
        std::clamp(config.getFloat("render.bloom_half_weight", bloomHalfWeight), 0.0F, 2.0F);
    bloomQuarterWeight =
        std::clamp(config.getFloat("render.bloom_quarter_weight", bloomQuarterWeight), 0.0F, 2.0F);
    hudGlassBlurRadiusPx =
        std::clamp(config.getFloat("render.hud_glass_blur_radius_px", hudGlassBlurRadiusPx), 0.0F, 24.0F);
    hudGlassRefractionRadiusPx = std::clamp(
        config.getFloat("render.hud_glass_refraction_radius_px", hudGlassRefractionRadiusPx),
        0.0F,
        24.0F);
    skidmarkMaxSegments =
        std::clamp(config.getInt("render.skidmark_max_segments", skidmarkMaxSegments), 256, 2048);
    cameraStartupShakeSuppressionS = std::clamp(
        config.getFloat("render.camera_startup_shake_suppression_s", cameraStartupShakeSuppressionS),
        0.0F,
        5.0F);
    cameraStartupShakeFadeS =
        std::clamp(config.getFloat("render.camera_startup_shake_fade_s", cameraStartupShakeFadeS), 0.0F, 3.0F);
    cameraAsphaltShake =
        std::clamp(config.getFloat("render.camera_asphalt_shake", cameraAsphaltShake), 0.0F, 0.05F);
    cameraApronShake =
        std::clamp(config.getFloat("render.camera_apron_shake", cameraApronShake), 0.0F, 0.08F);
    cameraGrassShake =
        std::clamp(config.getFloat("render.camera_grass_shake", cameraGrassShake), 0.0F, 0.12F);
    cameraRpmShake =
        std::clamp(config.getFloat("render.camera_rpm_shake", cameraRpmShake), 0.0F, 0.05F);
    cameraSpeedShake =
        std::clamp(config.getFloat("render.camera_speed_shake", cameraSpeedShake), 0.0F, 0.08F);
    cameraSlipStartUsage =
        std::clamp(config.getFloat("render.camera_slip_start_usage", cameraSlipStartUsage), 0.0F, 1.0F);
    cameraSlipShakeUsageRange = std::clamp(
        config.getFloat("render.camera_slip_shake_usage_range", cameraSlipShakeUsageRange),
        0.05F,
        1.0F);
    cameraSlipShake =
        std::clamp(config.getFloat("render.camera_slip_shake", cameraSlipShake), 0.0F, 0.15F);
    cameraTraumaShake =
        std::clamp(config.getFloat("render.camera_trauma_shake", cameraTraumaShake), 0.0F, 0.35F);
    cameraTraumaDecay =
        std::clamp(config.getFloat("render.camera_trauma_decay", cameraTraumaDecay), 0.2F, 8.0F);
    cameraBankRollScale =
        std::clamp(config.getFloat("render.camera_bank_roll_scale", cameraBankRollScale), 0.0F, 0.75F);
    cameraLateralGRollScale = std::clamp(
        config.getFloat("render.camera_lateral_g_roll_scale", cameraLateralGRollScale),
        0.0F,
        0.08F);
    cameraLongitudinalGRollScale = std::clamp(
        config.getFloat("render.camera_longitudinal_g_roll_scale", cameraLongitudinalGRollScale),
        0.0F,
        0.08F);
    activeTrack = config.getString("simulation.active_track", activeTrack);
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
