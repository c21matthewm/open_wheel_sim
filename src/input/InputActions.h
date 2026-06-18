#pragma once

namespace sim {

struct InputActions {
    float steer = 0.0F;
    float throttle = 0.0F;
    float brake = 0.0F;
    float clutch = 0.0F;
    bool shiftUp = false;
    bool shiftDown = false;
    bool resetCar = false;
    bool toggleMenu = false;
    bool menuUp = false;
    bool menuDown = false;
    bool menuLeft = false;
    bool menuRight = false;
    bool menuSelect = false;
    bool toggleFullscreen = false;
    bool toggleDiagnostics = false;
    bool toggleOverlay = false;
    bool toggleCamera = false;
    bool quit = false;
    bool wheelActive = false;
};

}  // namespace sim
