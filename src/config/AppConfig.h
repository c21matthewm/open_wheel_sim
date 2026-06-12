#pragma once

#include <string>

namespace sim {

class ConfigFile;

struct GraphicsConfig {
    std::string title = "Lightweight Open-Wheel Sim Prototype";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool borderless = false;
    bool vsync = true;
    float renderScale = 1.0F;
    int shadowMapSize = 2048;
    int physicsHz = 360;
    float maxFrameDelta = 0.1F;

    void load(const ConfigFile& config);
};

struct InputConfig {
    bool diagnosticsVisibleOnStart = true;
    float keyboardSteerRate = 3.2F;
    float keyboardReturnRate = 5.2F;

    bool wheelEnabled = true;
    std::string wheelNameContains = "G29";
    int steerAxis = 0;
    int throttleAxis = 2;
    int brakeAxis = 3;
    int shiftUpButton = 5;
    int shiftDownButton = 4;
    int cameraButton = 6;
    bool throttleInverted = true;
    bool brakeInverted = true;
    float steerDeadzone = 0.02F;
    float pedalDeadzone = 0.01F;
    float steerGamma = 1.0F;
    float wheelSteerSensitivity = 2.2F;
    float throttleGamma = 1.0F;
    float brakeGamma = 1.0F;

    void load(const ConfigFile& config);
};

}  // namespace sim
