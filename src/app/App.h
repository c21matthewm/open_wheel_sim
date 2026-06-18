#pragma once

#include <memory>

#include "audio/SoundManager.h"
#include "config/AppConfig.h"
#include "ffb/ForceFeedbackManager.h"
#include "game/RaceSession.h"
#include "input/InputManager.h"
#include "physics/Vehicle.h"
#include "platform/WindowSDL.h"
#include "render/MetalRenderer.h"
#include "render/RenderTypes.h"
#include "ui/DebugOverlay.h"

namespace sim {

class App {
public:
    App();
    int run();

private:
    bool initialize();
    void shutdown();
    void updateSmoothedStats(float frameSeconds, const PerformanceStats& measured);
    void handleMenuActions(const InputActions& actions);
    [[nodiscard]] InputActions drivingActionsFor(const InputActions& actions) const;
    [[nodiscard]] MenuOverlayState menuOverlayState() const;

    GraphicsConfig graphicsConfig_;
    InputConfig inputConfig_;
    VehicleConfig vehicleConfig_;
    TrackConfig trackConfig_;
    ForceFeedbackConfig forceFeedbackConfig_;
    WindowSDL window_;
    InputManager input_;
    SoundManager sound_;
    ForceFeedbackManager forceFeedback_;
    MetalRenderer renderer_;
    std::unique_ptr<Vehicle> vehicle_;
    std::unique_ptr<RaceSession> raceSession_;
    std::unique_ptr<DebugOverlay> overlay_;
    PerformanceStats stats_;
    bool running_ = false;
    bool menuVisible_ = false;
    float benchmarkDurationSeconds_ = 0.0F;
    float benchmarkElapsedSeconds_ = 0.0F;
    double benchmarkFrameMsTotal_ = 0.0;
    double benchmarkPhysicsMsTotal_ = 0.0;
    double benchmarkRenderMsTotal_ = 0.0;
    int benchmarkFrames_ = 0;
    int benchmarkPhysicsSteps_ = 0;
    int selectedMenuItem_ = 0;
    int cameraMode_ = 0;
};

}  // namespace sim
