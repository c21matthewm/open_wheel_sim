#include "ui/DebugOverlay.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sim {
namespace {

constexpr OverlayColor kHeading{0.35F, 0.85F, 1.0F, 1.0F};
constexpr OverlayColor kPrimary{0.92F, 0.95F, 1.0F, 1.0F};
constexpr OverlayColor kDim{0.65F, 0.7F, 0.76F, 1.0F};
constexpr OverlayColor kActive{0.45F, 1.0F, 0.45F, 1.0F};
constexpr OverlayColor kWarning{1.0F, 0.78F, 0.35F, 1.0F};
constexpr float kLineHeight = 11.0F;

float degrees(float radians) {
    return radians * 57.2957795F;
}

struct TimeParts {
    int minutes = 0;
    float seconds = 0.0F;
};

TimeParts splitTime(float totalSeconds) {
    const float clamped = std::max(0.0F, totalSeconds);
    const int minutes = static_cast<int>(clamped / 60.0F);
    return {minutes, clamped - static_cast<float>(minutes) * 60.0F};
}

const char* engineMapName(int engineMap) {
    switch (engineMap) {
        case 0:
            return "LEAN";
        case 2:
            return "RICH";
        case 1:
        default:
            return "STANDARD";
    }
}

float estimatedFuelLapsRemaining(const VehicleState& vehicle, const RaceTelemetry& race) {
    const float referenceSpeedMps = std::max(65.0F, vehicle.speedMps);
    const float estimatedLapSeconds =
        race.lap.bestLapSeconds > 1.0F
            ? race.lap.bestLapSeconds
            : race.lapLengthM / referenceSpeedMps;
    const float burnRate = std::max(
        {vehicle.fuelAverageBurnRateGps, vehicle.fuelBurnRateGps * 0.35F, 0.00001F});
    const float gallonsPerLap = std::max(0.0001F, burnRate * estimatedLapSeconds);
    return std::clamp(vehicle.fuelGallons / gallonsPerLap, 0.0F, 99.99F);
}

}  // namespace

DebugOverlay::DebugOverlay(bool diagnosticsVisible) : diagnosticsVisible_(diagnosticsVisible) {}

void DebugOverlay::build(
    const PerformanceStats& stats,
    const VehicleState& vehicle,
    const RaceTelemetry& race,
    const InputActions& input,
    const SDLJoystickInput& joysticks,
    int pixelWidth,
    int pixelHeight,
    const MenuOverlayState& menu,
    const char* ffbStatus) {
    lineCount_ = 0;

    const float hudY = std::max(112.0F, static_cast<float>(pixelHeight) - 150.0F);
    const float centerX = static_cast<float>(pixelWidth) * 0.5F;
    const float hudLeftTextX = std::max(18.0F, centerX - 438.0F);
    const float hudRightTextX = std::min(centerX + 96.0F, static_cast<float>(pixelWidth) - 330.0F);
    const OverlayColor speedColor{0.86F, 0.94F, 1.0F, 0.95F};
    const OverlayColor gearColor{1.0F, 0.98F, 0.82F, 1.0F};
    const OverlayColor rpmColor = vehicle.rpm > 11250.0F
                                      ? OverlayColor{1.0F, 0.18F, 0.12F, 1.0F}
                                      : OverlayColor{0.62F, 1.0F, 0.72F, 0.95F};
    addScaled(hudLeftTextX, hudY + 54.0F, 2.04F, speedColor, "%03.0F MPH", vehicle.speedMps * 2.2369363F);
    addScaled(centerX - 32.0F, hudY + 42.0F, 3.75F, gearColor, "%d", vehicle.gear);
    addScaled(hudRightTextX, hudY + 42.0F, 1.58F, rpmColor, "%05.0F RPM", vehicle.rpm);
    addScaled(hudRightTextX, hudY + 68.0F, 1.00F, kDim, "LAT %+3.1FG  LONG %+3.1FG", vehicle.lateralG, vehicle.longitudinalG);
    addScaled(hudLeftTextX, hudY + 99.0F, 0.98F, kDim, "BRAKE        THROTTLE");
    addScaled(hudRightTextX, hudY + 92.0F, 1.02F, kDim, "FUEL: %.2F LAPS", estimatedFuelLapsRemaining(vehicle, race));
    addScaled(hudRightTextX, hudY + 111.0F, 1.02F, kDim, "MAP: %s", engineMapName(vehicle.engineMap));

    if (menu.visible) {
        const float menuX = centerX - 185.0F;
        float menuY = 98.0F;
        const auto itemColor = [&menu](int index) {
            return menu.selectedIndex == index ? kActive : kPrimary;
        };
        addScaled(menuX, menuY, 1.45F, kHeading, "ESC MENU");
        menuY += 24.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(0), "%s KEYBOARD STEER RATE  %4.2F",
            menu.selectedIndex == 0 ? ">" : " ", menu.keyboardSteerRate);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(1), "%s KEYBOARD RETURN RATE %4.2F",
            menu.selectedIndex == 1 ? ">" : " ", menu.keyboardReturnRate);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(2), "%s FRONT WING          %+4.0F",
            menu.selectedIndex == 2 ? ">" : " ", menu.frontWingSetting);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(3), "%s REAR WING           %+4.0F",
            menu.selectedIndex == 3 ? ">" : " ", menu.rearWingSetting);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(4), "%s BRAKE BIAS           %4.2F",
            menu.selectedIndex == 4 ? ">" : " ", menu.brakeBias);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(5), "%s ENGINE MAP          %s",
            menu.selectedIndex == 5 ? ">" : " ", menu.engineMapName);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(6), "%s AUTOMATIC TRANS     %s",
            menu.selectedIndex == 6 ? ">" : " ", menu.automaticShift ? "ON" : "OFF");
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(7), "%s AERO PRESET         %s",
            menu.selectedIndex == 7 ? ">" : " ", menu.aeroPresetName);
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(8), "%s RESET RECORDS/GHOST  %s",
            menu.selectedIndex == 8 ? ">" : " ", menu.ghostAvailable ? "READY" : "EMPTY");
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(9), "%s CLOSE MENU",
            menu.selectedIndex == 9 ? ">" : " ");
        menuY += 18.0F;
        addScaled(menuX, menuY, 1.08F, itemColor(10), "%s QUIT TO HOME",
            menu.selectedIndex == 10 ? ">" : " ");
        menuY += 24.0F;
        addScaled(menuX, menuY, 0.88F, kDim, "UP/DOWN SELECT  LEFT/RIGHT ADJUST  ENTER ACTIVATE");
    }

    if (!visible_) {
        return;
    }

    float y = 12.0F;
    add(12.0F, y, kHeading, "LIGHTWEIGHT OPEN-WHEEL SIM  |  F1 OVERLAY  F2 DEVICES  F11 FULLSCREEN");
    y += kLineHeight * 1.5F;
    add(12.0F, y, kPrimary, "FPS %5.1F  FRAME %5.2F MS  PHYS %5.2F MS (%d)  RENDER %5.2F MS  INPUT %5.2F MS",
        stats.fps, stats.frameMs, stats.physicsMs, stats.physicsSteps, stats.renderMs, stats.inputMs);
    y += kLineHeight;
    add(12.0F, y, kPrimary, "SPEED %6.1F KM/H  GEAR %d  RPM %5.0F  POS X %7.1F Z %7.1F",
        vehicle.speedMps * 3.6F, vehicle.gear, vehicle.rpm, vehicle.positionX, vehicle.positionZ);
    y += kLineHeight;
    add(12.0F, y, input.wheelActive ? kActive : kWarning,
        "INPUT %s  STEER %+5.2F (%+5.1F DEG)  THROTTLE %4.2F  BRAKE %4.2F",
        input.wheelActive ? "WHEEL" : "KEYBOARD", input.steer, degrees(vehicle.steeringAngleRadians),
        input.throttle, input.brake);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "SLIP FRONT %+6.2F DEG  REAR %+6.2F DEG  TRAIL F/R %.3F/%.3F M  LAT G %+5.2F  LONG G %+5.2F  %s",
        degrees(vehicle.frontSlipAngleRadians), degrees(vehicle.rearSlipAngleRadians),
        vehicle.frontPneumaticTrailM, vehicle.rearPneumaticTrailM, vehicle.lateralG, vehicle.longitudinalG,
        ffbStatus != nullptr ? ffbStatus : "FFB: unknown");
    y += kLineHeight;
    add(12.0F, y, kDim,
        "4W USE FL/FR/RL/RR %4.2F %4.2F %4.2F %4.2F  LONG %+4.2F %+4.2F %+4.2F %+4.2F",
        vehicle.frontLeftTireUsage, vehicle.frontRightTireUsage, vehicle.rearLeftTireUsage,
        vehicle.rearRightTireUsage, vehicle.frontLeftLongitudinalSlip,
        vehicle.frontRightLongitudinalSlip, vehicle.rearLeftLongitudinalSlip,
        vehicle.rearRightLongitudinalSlip);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "TIRE TEMP FL/FR/RL/RR %4.0F %4.0F %4.0F %4.0F C  THERMAL GRIP %4.2F %4.2F %4.2F %4.2F",
        vehicle.frontLeftTireTemperatureC, vehicle.frontRightTireTemperatureC,
        vehicle.rearLeftTireTemperatureC, vehicle.rearRightTireTemperatureC,
        vehicle.frontLeftThermalGrip, vehicle.frontRightThermalGrip,
        vehicle.rearLeftThermalGrip, vehicle.rearRightThermalGrip);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "SLIP RATIO FL/FR/RL/RR %+5.2F %+5.2F %+5.2F %+5.2F",
        vehicle.frontLeftLongitudinalSlip, vehicle.frontRightLongitudinalSlip,
        vehicle.rearLeftLongitudinalSlip, vehicle.rearRightLongitudinalSlip);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "TIRE LIFE FL/FR/RL/RR %3.0F %3.0F %3.0F %3.0F%%  FUEL %.2F GAL  BURN %.3F GPS  MAP %s",
        vehicle.frontLeftTireWear * 100.0F, vehicle.frontRightTireWear * 100.0F,
        vehicle.rearLeftTireWear * 100.0F, vehicle.rearRightTireWear * 100.0F,
        vehicle.fuelGallons, vehicle.fuelBurnRateGps, engineMapName(vehicle.engineMap));
    y += kLineHeight;
    add(12.0F, y, kDim,
        "LOAD FL/FR/RL/RR %4.1F %4.1F %4.1F %4.1F KN  AERO %4.1F KN  BRAKE BIAS %4.2F",
        vehicle.frontLeftNormalLoadN * 0.001F, vehicle.frontRightNormalLoadN * 0.001F,
        vehicle.rearLeftNormalLoadN * 0.001F, vehicle.rearRightNormalLoadN * 0.001F,
        vehicle.downforceN * 0.001F, vehicle.brakeBias);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "ROLL K F/R %5.1F/%5.1F KNM/RAD  FRONT ROLL BAL %4.2F",
        vehicle.frontRollStiffnessNmPerRad * 0.001F,
        vehicle.rearRollStiffnessNmPerRad * 0.001F,
        vehicle.frontRollStiffnessFraction);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "CHASSIS HEAVE %+4.2F CM  PITCH %+5.2F DEG  ROLL %+5.2F DEG  RH F/R %4.1F/%4.1F CM  COP %4.2F  TRAY %3.0F%%",
        vehicle.chassisHeaveM * 100.0F, degrees(vehicle.chassisPitchRadians),
        degrees(vehicle.chassisRollRadians), vehicle.frontRideHeightM * 100.0F,
        vehicle.rearRideHeightM * 100.0F, vehicle.aeroCenterOfPressure,
        vehicle.floorStrikeIntensity * 100.0F);
    y += kLineHeight;
    add(12.0F, y, kDim,
        "FORCES DRIVE %5.1F KN  BRAKE %5.1F KN  DRAG %+5.1F KN  LAT F/R %+5.1F/%+5.1F KN",
        vehicle.engineForceN * 0.001F, vehicle.brakeForceN * 0.001F,
        vehicle.dragForceN * 0.001F, vehicle.frontLateralForceN * 0.001F,
        vehicle.rearLateralForceN * 0.001F);
    y += kLineHeight;
    add(12.0F, y, race.surface == TrackSurface::Asphalt ? kActive : kWarning,
        "SURFACE %s  GRIP LAT/LONG %4.2F/%4.2F  OFFSET %+6.1F M  PROGRESS %5.1F%%  BANK %4.1F DEG%s",
        trackSurfaceName(race.surface), race.gripMultiplier, race.longitudinalGripMultiplier, race.lateralOffsetM,
        race.progressM / race.lapLengthM * 100.0F, degrees(race.bankRadians),
        race.wallContact ? "  WALL CONTACT" : "");
    y += kLineHeight;

    const TimeParts current = splitTime(race.lap.currentLapSeconds);
    const TimeParts last = splitTime(race.lap.lastLapSeconds);
    const TimeParts best = splitTime(race.lap.bestLapSeconds);
    if (race.lap.active) {
        add(12.0F, y, race.lap.currentLapValid ? kPrimary : kWarning,
            "LAP %d  CURRENT %02d:%06.3F  LAST %02d:%06.3F%s  BEST %02d:%06.3F  NEXT CP %d",
            race.lap.lapNumber, current.minutes, current.seconds, last.minutes, last.seconds,
            race.lap.lastLapValid ? "" : " INVALID", best.minutes, best.seconds,
            race.lap.nextCheckpoint);
    } else {
        add(12.0F, y, kWarning, "OUT LAP  CROSS START/FINISH TO BEGIN TIMING");
    }
    y += kLineHeight * 1.5F;
    add(12.0F, y, kDim, "DRIVE: WASD/ARROWS  BRAKE: S/SPACE  CAMERA: C/WHEEL BTN  RESET: R  MENU: ESC");

    if (!diagnosticsVisible_) {
        return;
    }

    const bool useSidePanel = pixelWidth >= 1150;
    const float x = useSidePanel ? static_cast<float>(pixelWidth) * 0.47F : 12.0F;
    y = useSidePanel ? 48.0F : 176.0F;
    add(x, y, kHeading, "SDL3 INPUT DIAGNOSTICS");
    y += kLineHeight;
    add(x, y, joysticks.deviceCount() > 0 ? kActive : kWarning, "CONNECTED DEVICES: %d", joysticks.deviceCount());
    y += kLineHeight * 1.5F;

    for (int deviceIndex = 0; deviceIndex < joysticks.deviceCount(); ++deviceIndex) {
        const auto& device = joysticks.device(deviceIndex);
        add(x, y, kPrimary, "[%d] %s", deviceIndex, device.name.data());
        y += kLineHeight;
        add(x, y, kDim, "VID %04X PID %04X  AXES %d  BUTTONS %d  HATS %d  GAMEPAD %s  HAPTIC %s",
            device.vendor, device.product, device.axisCount, device.buttonCount, device.hatCount,
            device.isGamepad ? "YES" : "NO", device.isHaptic ? "YES" : "NO");
        y += kLineHeight;

        for (int firstAxis = 0; firstAxis < device.axisCount; firstAxis += 4) {
            const float a0 = device.axes[firstAxis];
            const float a1 = firstAxis + 1 < device.axisCount ? device.axes[firstAxis + 1] : 0.0F;
            const float a2 = firstAxis + 2 < device.axisCount ? device.axes[firstAxis + 2] : 0.0F;
            const float a3 = firstAxis + 3 < device.axisCount ? device.axes[firstAxis + 3] : 0.0F;
            switch (device.axisCount - firstAxis) {
                case 1:
                    add(x, y, kDim, "AXIS %02d %+6.3F", firstAxis, a0);
                    break;
                case 2:
                    add(x, y, kDim, "AXIS %02d %+6.3F  %02d %+6.3F",
                        firstAxis, a0, firstAxis + 1, a1);
                    break;
                case 3:
                    add(x, y, kDim, "AXIS %02d %+6.3F  %02d %+6.3F  %02d %+6.3F",
                        firstAxis, a0, firstAxis + 1, a1, firstAxis + 2, a2);
                    break;
                default:
                    add(x, y, kDim, "AXIS %02d %+6.3F  %02d %+6.3F  %02d %+6.3F  %02d %+6.3F",
                        firstAxis, a0, firstAxis + 1, a1, firstAxis + 2, a2, firstAxis + 3, a3);
                    break;
            }
            y += kLineHeight;
        }

        int pressedCount = 0;
        std::array<char, 128> pressed{};
        int written = std::snprintf(pressed.data(), pressed.size(), "PRESSED:");
        for (int button = 0; button < device.buttonCount && written > 0 &&
                             written < static_cast<int>(pressed.size()) - 5; ++button) {
            if (device.buttons[button]) {
                written += std::snprintf(
                    pressed.data() + written, pressed.size() - static_cast<std::size_t>(written), " %d", button);
                ++pressedCount;
            }
        }
        add(x, y, pressedCount > 0 ? kActive : kDim, "%s%s", pressed.data(), pressedCount == 0 ? " NONE" : "");
        y += kLineHeight * 1.5F;
    }
}

}  // namespace sim
