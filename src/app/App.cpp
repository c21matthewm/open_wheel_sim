#include "app/App.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <SDL3/SDL.h>

#include "app/GameLoop.h"
#include "config/ConfigFile.h"
#include "platform/FileSystem.h"

namespace sim {
namespace {

using Clock = std::chrono::steady_clock;

float milliseconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<float, std::milli>(end - start).count();
}

bool loadConfig(const char* relativePath, ConfigFile& config) {
    const std::string path = FileSystem::findDataFile(relativePath);
    if (!config.load(path)) {
        std::fprintf(stderr, "%s\n", config.error().c_str());
        return false;
    }
    return true;
}

}  // namespace

App::App() = default;

int App::run() {
    if (!initialize()) {
        shutdown();
        return 1;
    }

    GameLoop gameLoop(1.0F / static_cast<float>(graphicsConfig_.physicsHz), graphicsConfig_.maxFrameDelta);
    running_ = true;

    while (running_) {
        const Clock::time_point frameStart = Clock::now();
        const float frameSeconds = gameLoop.beginFrame();
        PerformanceStats measured;
        measured.frameMs = frameSeconds * 1000.0F;

        const Clock::time_point inputStart = Clock::now();
        input_.beginFrame();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            window_.handleEvent(event);
            input_.handleEvent(event);
        }
        input_.setVehicleSpeed(vehicle_->current().speedMps);
        input_.update(frameSeconds);
        const InputActions& actions = input_.actions();
        if (actions.quit) {
            running_ = false;
        }
        if (actions.toggleMenu) {
            menuVisible_ = !menuVisible_;
        }
        if (actions.toggleFullscreen) {
            window_.toggleFullscreen();
        }
        if (actions.toggleDiagnostics) {
            overlay_->toggleDiagnostics();
        }
        if (actions.toggleOverlay) {
            overlay_->toggleVisible();
        }
        if (actions.toggleCamera && !menuVisible_) {
            cameraMode_ = cameraMode_ == 0 ? 1 : 0;
        }
        handleMenuActions(actions);
        if (menuVisible_) {
            input_.centerKeyboardSteer();
        }
        if (actions.resetCar && !menuVisible_) {
            raceSession_->resetVehicle(*vehicle_);
        }
        const InputActions drivingActions = drivingActionsFor(actions);
        measured.inputMs = milliseconds(inputStart, Clock::now());

        raceSession_->beginRenderFrame();
        const Clock::time_point physicsStart = Clock::now();
        while (gameLoop.shouldStep()) {
            if (!menuVisible_) {
                raceSession_->step(*vehicle_, drivingActions, gameLoop.fixedDeltaSeconds());
                forceFeedback_.update(
                    vehicle_->current(),
                    raceSession_->telemetry(),
                    drivingActions,
                    input_.selectedWheel(),
                    gameLoop.fixedDeltaSeconds());
                ++measured.physicsSteps;
            } else {
                forceFeedback_.update(
                    vehicle_->current(),
                    raceSession_->telemetry(),
                    drivingActions,
                    input_.selectedWheel(),
                    gameLoop.fixedDeltaSeconds());
            }
            gameLoop.consumeStep();
        }
        measured.physicsMs = milliseconds(physicsStart, Clock::now());

        const RaceTelemetry& telemetry = raceSession_->telemetry();
        const float lapDeltaSeconds =
            telemetry.lap.bestLapSeconds > 0.0F
                ? telemetry.lap.currentLapSeconds - telemetry.lap.bestLapSeconds
                : 0.0F;
        const RenderScene scene{
            vehicle_->interpolated(gameLoop.interpolationAlpha()),
            telemetry.vehicleHeightM,
            telemetry.bankRadians,
            {telemetry.ghost.available,
             telemetry.ghost.positionX,
             telemetry.ghost.positionZ,
             telemetry.ghost.yawRadians,
             telemetry.ghost.heightM,
             telemetry.ghost.bankRadians},
            drivingActions.brake,
            drivingActions.throttle,
            telemetry.surface,
            telemetry.wallContact,
            window_.pixelWidth(),
            window_.pixelHeight(),
            telemetry.lap.currentLapSeconds,
            telemetry.lap.bestLapSeconds,
            lapDeltaSeconds,
            cameraMode_,
        };
        sound_.update(scene.vehicle, drivingActions, telemetry);
        overlay_->build(
            stats_,
            scene.vehicle,
            telemetry,
            drivingActions,
            input_.joysticks(),
            scene.pixelWidth,
            scene.pixelHeight,
            menuOverlayState(),
            forceFeedback_.status().c_str());

        const Clock::time_point renderStart = Clock::now();
        if (!renderer_.render(scene, *overlay_)) {
            std::fprintf(stderr, "Render failure: %s\n", renderer_.error().c_str());
            running_ = false;
        }
        measured.renderMs = milliseconds(renderStart, Clock::now());
        measured.frameMs = milliseconds(frameStart, Clock::now());
        updateSmoothedStats(frameSeconds, measured);
        if (benchmarkDurationSeconds_ > 0.0F) {
            benchmarkElapsedSeconds_ += measured.frameMs * 0.001F;
            benchmarkFrameMsTotal_ += measured.frameMs;
            benchmarkPhysicsMsTotal_ += measured.physicsMs;
            benchmarkRenderMsTotal_ += measured.renderMs;
            benchmarkPhysicsSteps_ += measured.physicsSteps;
            ++benchmarkFrames_;
            if (benchmarkElapsedSeconds_ >= benchmarkDurationSeconds_) {
                const double frames = std::max(1, benchmarkFrames_);
                const double elapsed = std::max(0.001, static_cast<double>(benchmarkElapsedSeconds_));
                std::fprintf(
                    stderr,
                    "BENCHMARK %.2fs: FPS %.1f  FRAME %.2f ms  PHYS %.3f ms  RENDER %.2f ms  PHYS_STEPS %.1f/s\n",
                    benchmarkElapsedSeconds_,
                    static_cast<double>(benchmarkFrames_) / elapsed,
                    benchmarkFrameMsTotal_ / frames,
                    benchmarkPhysicsMsTotal_ / frames,
                    benchmarkRenderMsTotal_ / frames,
                    static_cast<double>(benchmarkPhysicsSteps_) / elapsed);
                running_ = false;
            }
        }
    }

    shutdown();
    return 0;
}

bool App::initialize() {
    ConfigFile graphicsFile;
    if (!loadConfig("config/graphics_default.json", graphicsFile)) {
        return false;
    }
    graphicsConfig_.load(graphicsFile);

    ConfigFile inputFile;
    if (!loadConfig("config/input_default.json", inputFile)) {
        return false;
    }
    inputConfig_.load(inputFile);

    ConfigFile vehicleFile;
    if (!loadConfig("config/vehicle_openwheel_default.json", vehicleFile)) {
        return false;
    }
    vehicleConfig_.load(vehicleFile);

    ConfigFile trackFile;
    if (!loadConfig("config/track_oval_default.json", trackFile)) {
        return false;
    }
    trackConfig_.load(trackFile);

    ConfigFile ffbFile;
    if (!loadConfig("config/ffb_default.json", ffbFile)) {
        return false;
    }
    forceFeedbackConfig_.load(ffbFile);

    if (const char* benchmarkSeconds = std::getenv("SIM_BENCHMARK_SECONDS");
        benchmarkSeconds != nullptr) {
        benchmarkDurationSeconds_ =
            std::clamp(std::strtof(benchmarkSeconds, nullptr), 0.0F, 120.0F);
    }

    if (!window_.initialize(graphicsConfig_)) {
        std::fprintf(stderr, "Window initialization failed: %s\n", window_.error().c_str());
        return false;
    }
    input_.initialize(inputConfig_);
    input_.setKeyboardHighSpeedResponse(
        vehicleConfig_.highSpeedSteerScale,
        vehicleConfig_.steerSpeedThresholdMps);
    if (!sound_.initialize()) {
        std::fprintf(stderr, "Audio disabled: %s\n", sound_.error().c_str());
    }
    forceFeedback_.initialize(forceFeedbackConfig_);
    vehicle_ = std::make_unique<Vehicle>(vehicleConfig_);
    vehicle_->setAeroPreset(0);  // Default to speedway aero for oval
    raceSession_ = std::make_unique<RaceSession>(trackConfig_);
    raceSession_->resetVehicle(*vehicle_);
    overlay_ = std::make_unique<DebugOverlay>(inputConfig_.diagnosticsVisibleOnStart);

    if (!renderer_.initialize(
            window_,
            graphicsConfig_.vsync,
            graphicsConfig_.renderScale,
            graphicsConfig_.shadowMapSize,
            graphicsConfig_.shadowUpdateInterval,
            graphicsConfig_.msaaSamples,
            graphicsConfig_.shadowFrustumExtentM,
            graphicsConfig_.shadowLightDistanceM,
            graphicsConfig_.shadowLightHeightOffsetM,
            graphicsConfig_.bloomHalfWeight,
            graphicsConfig_.bloomQuarterWeight,
            graphicsConfig_.hudGlassBlurRadiusPx,
            graphicsConfig_.hudGlassRefractionRadiusPx,
            graphicsConfig_.cameraStartupShakeSuppressionS,
            graphicsConfig_.cameraStartupShakeFadeS,
            graphicsConfig_.cameraAsphaltShake,
            graphicsConfig_.cameraApronShake,
            graphicsConfig_.cameraGrassShake,
            graphicsConfig_.cameraRpmShake,
            graphicsConfig_.cameraSpeedShake,
            graphicsConfig_.cameraSlipStartUsage,
            graphicsConfig_.cameraSlipShakeUsageRange,
            graphicsConfig_.cameraSlipShake,
            graphicsConfig_.cameraTraumaShake,
            graphicsConfig_.cameraTraumaDecay,
            graphicsConfig_.cameraBankRollScale,
            graphicsConfig_.cameraLateralGRollScale,
            graphicsConfig_.cameraLongitudinalGRollScale,
            raceSession_->track())) {
        std::fprintf(stderr, "Renderer initialization failed: %s\n", renderer_.error().c_str());
        return false;
    }
    return true;
}

void App::shutdown() {
    renderer_.shutdown();
    sound_.shutdown();
    forceFeedback_.shutdown();
    input_.shutdown();
    overlay_.reset();
    raceSession_.reset();
    vehicle_.reset();
    window_.shutdown();
}

void App::handleMenuActions(const InputActions& actions) {
    if (!menuVisible_) {
        return;
    }

    constexpr int kMenuItemCount = 7;
    if (actions.menuUp) {
        selectedMenuItem_ = (selectedMenuItem_ + kMenuItemCount - 1) % kMenuItemCount;
    }
    if (actions.menuDown) {
        selectedMenuItem_ = (selectedMenuItem_ + 1) % kMenuItemCount;
    }

    const int adjustment = (actions.menuRight ? 1 : 0) - (actions.menuLeft ? 1 : 0);
    if (adjustment != 0) {
        switch (selectedMenuItem_) {
            case 0:
                inputConfig_.keyboardSteerRate =
                    std::clamp(inputConfig_.keyboardSteerRate + 0.25F * static_cast<float>(adjustment), 0.5F, 10.0F);
                input_.setKeyboardRates(inputConfig_.keyboardSteerRate, inputConfig_.keyboardReturnRate);
                break;
            case 1:
                inputConfig_.keyboardReturnRate =
                    std::clamp(inputConfig_.keyboardReturnRate + 0.25F * static_cast<float>(adjustment), 0.5F, 12.0F);
                input_.setKeyboardRates(inputConfig_.keyboardSteerRate, inputConfig_.keyboardReturnRate);
                break;
            case 2:
                vehicle_->setBrakeBias(
                    vehicle_->brakeBias() + 0.01F * static_cast<float>(adjustment));
                break;
            case 3:
                vehicle_->setAutomaticShift(!vehicle_->automaticShift());
                break;
            case 4:
                vehicle_->setAeroPreset((vehicle_->aeroPreset() + 1) % 2);
                break;
            default:
                break;
        }
    }

    if (!actions.menuSelect) {
        return;
    }
    if (selectedMenuItem_ == 3) {
        vehicle_->setAutomaticShift(!vehicle_->automaticShift());
    } else if (selectedMenuItem_ == 4) {
        vehicle_->setAeroPreset((vehicle_->aeroPreset() + 1) % 2);
    } else if (selectedMenuItem_ == 5) {
        raceSession_->resetRecords();
    } else if (selectedMenuItem_ == 6) {
        menuVisible_ = false;
    }
}

InputActions App::drivingActionsFor(const InputActions& actions) const {
    InputActions driving = actions;
    if (menuVisible_) {
        driving.steer = 0.0F;
        driving.throttle = 0.0F;
        driving.brake = 0.0F;
        driving.clutch = 0.0F;
        driving.shiftUp = false;
        driving.shiftDown = false;
        driving.resetCar = false;
    }
    return driving;
}

MenuOverlayState App::menuOverlayState() const {
    MenuOverlayState state;
    state.visible = menuVisible_;
    state.selectedIndex = selectedMenuItem_;
    state.keyboardSteerRate = inputConfig_.keyboardSteerRate;
    state.keyboardReturnRate = inputConfig_.keyboardReturnRate;
    state.brakeBias = vehicle_ != nullptr ? vehicle_->brakeBias() : vehicleConfig_.brakeBias;
    state.automaticShift =
        vehicle_ != nullptr ? vehicle_->automaticShift() : vehicleConfig_.automaticShift;
    state.ghostAvailable =
        raceSession_ != nullptr && raceSession_->telemetry().ghost.available;
    state.aeroPresetName =
        vehicle_ != nullptr ? vehicle_->aeroPresetName() : "SPEEDWAY";
    return state;
}

void App::updateSmoothedStats(float frameSeconds, const PerformanceStats& measured) {
    constexpr float smoothing = 0.08F;
    const auto smooth = [](float previous, float current) {
        return previous == 0.0F ? current : previous + (current - previous) * smoothing;
    };
    const float instantaneousFps = frameSeconds > 0.00001F ? 1.0F / frameSeconds : 0.0F;
    stats_.fps = smooth(stats_.fps, instantaneousFps);
    stats_.frameMs = smooth(stats_.frameMs, measured.frameMs);
    stats_.inputMs = smooth(stats_.inputMs, measured.inputMs);
    stats_.physicsMs = smooth(stats_.physicsMs, measured.physicsMs);
    stats_.renderMs = smooth(stats_.renderMs, measured.renderMs);
    stats_.physicsSteps = measured.physicsSteps;
}

}  // namespace sim
