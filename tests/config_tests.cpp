#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

#include "config/AppConfig.h"
#include "config/ConfigFile.h"
#include "physics/Vehicle.h"
#include "physics/VehicleConfig.h"

namespace {

bool near(float lhs, float rhs, float epsilon = 0.001F) {
    return std::abs(lhs - rhs) <= epsilon;
}

constexpr int kPhysicsHz = 360;
constexpr float kPhysicsDt = 1.0F / static_cast<float>(kPhysicsHz);
constexpr float kSecondGearLaunchUpshiftMinSpeedMps = 22.0F;
constexpr float kMpsToMph = 2.23693629F;

int stepsForSeconds(float seconds) {
    return static_cast<int>(std::lround(seconds * static_cast<float>(kPhysicsHz)));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Expected config directory argument\n";
        return 1;
    }

    const std::string configDirectory = argv[1];
    sim::ConfigFile config;
    if (!config.load(configDirectory + "/vehicle_openwheel_default.json")) {
        std::cerr << config.error() << '\n';
        return 1;
    }

    if (!near(config.getFloat("body.mass_kg", 0.0F), 733.0F) ||
        !config.getBool("powertrain.automatic_transmission", false)) {
        std::cerr << "Nested config lookup failed\n";
        return 1;
    }

    sim::ConfigFile graphicsConfigFile;
    if (!graphicsConfigFile.load(configDirectory + "/graphics_default.json")) {
        std::cerr << graphicsConfigFile.error() << '\n';
        return 1;
    }
    sim::GraphicsConfig graphicsConfig;
    graphicsConfig.load(graphicsConfigFile);
    if (graphicsConfig.physicsHz != kPhysicsHz) {
        std::cerr << "Physics tick config did not load at 360 Hz\n";
        return 1;
    }

    sim::ConfigFile inputConfigFile;
    if (!inputConfigFile.load(configDirectory + "/input_default.json")) {
        std::cerr << inputConfigFile.error() << '\n';
        return 1;
    }
    sim::InputConfig inputConfig;
    inputConfig.load(inputConfigFile);
    if (!inputConfig.throttleInverted ||
        !inputConfig.brakeInverted ||
        !near(inputConfig.keyboardSteerRate, 3.2F) ||
        !near(inputConfig.keyboardReturnRate, 5.2F) ||
        !near(inputConfig.steerGamma, 1.0F) ||
        !near(inputConfig.wheelSteerSensitivity, 2.2F) ||
        !near(inputConfig.throttleGamma, 1.0F) ||
        !near(inputConfig.brakeGamma, 1.0F)) {
        std::cerr << "Expanded input calibration config lookup failed\n";
        return 1;
    }

    sim::VehicleConfig vehicleConfig;
    vehicleConfig.load(config);
    if (!near(vehicleConfig.centerOfMassHeightM, 0.30F) ||
        !near(vehicleConfig.brakeBias, 0.58F) ||
        !near(vehicleConfig.downforceNPerMps2, 1.95F) ||
        !near(vehicleConfig.finalDriveRatio, 3.65F) ||
        !near(vehicleConfig.gearRatios[0], 3.55345F) ||
        !near(vehicleConfig.gearRatios[5], 1.105F) ||
        !near(vehicleConfig.lateralPeakSlipAngleRadians, 6.8F * std::numbers::pi_v<float> / 180.0F) ||
        !near(vehicleConfig.longitudinalPeakSlipRatio, 0.105F) ||
        !near(vehicleConfig.tireLoadSensitivityCoeff, 0.10F) ||
        !near(vehicleConfig.tireLoadSensitivityMinEfficiency, 0.65F) ||
        !near(vehicleConfig.tireLoadReferenceNormalN, 1500.0F) ||
        !near(vehicleConfig.tireCamberStiffnessNPerRad, 1000.0F) ||
        !near(vehicleConfig.camberAngleFrontRadians, -0.052F) ||
        !near(vehicleConfig.camberAngleRearRadians, -0.017F) ||
        !near(vehicleConfig.roadCourseCamberAngleFrontRadians, -0.070F) ||
        !near(vehicleConfig.roadCourseCamberAngleRearRadians, -0.030F) ||
        !near(vehicleConfig.tireRelaxationLengthM, 0.12F) ||
        !near(vehicleConfig.frontSpringRateNPerM, 100000.0F) ||
        !near(vehicleConfig.rearStaticRideHeightM, 0.060F) ||
        !near(vehicleConfig.wheelInertiaKgM2, 1.25F) ||
        !near(vehicleConfig.maxGroundEffectMultiplier, 2.90F) ||
        !near(vehicleConfig.aeroBrakeCopShift, 0.035F) ||
        !near(vehicleConfig.aeroInstantLoadFraction, 0.16F) ||
        !near(vehicleConfig.frontRollStiffnessFraction, 0.52F) ||
        !near(vehicleConfig.highSpeedSteerScale, 0.22F) ||
        !near(vehicleConfig.steerSpeedThresholdMps, 75.0F) ||
        !near(vehicleConfig.driveFrontFraction, 0.0F) ||
        !vehicleConfig.automaticShift) {
        std::cerr << "Expanded vehicle config lookup failed\n";
        return 1;
    }
    const float sixthGearRedlineMps =
        vehicleConfig.redlineRpm * 2.0F * std::numbers::pi_v<float> / 60.0F *
        vehicleConfig.wheelRadiusM /
        (vehicleConfig.finalDriveRatio * vehicleConfig.gearRatios[5]);
    const float fourthGearRedlineMph =
        vehicleConfig.redlineRpm * 2.0F * std::numbers::pi_v<float> / 60.0F *
        vehicleConfig.wheelRadiusM /
        (vehicleConfig.finalDriveRatio * vehicleConfig.gearRatios[3]) * kMpsToMph;
    if (sixthGearRedlineMps < 106.0F || sixthGearRedlineMps > 107.3F ||
        fourthGearRedlineMph < 145.0F || fourthGearRedlineMph > 154.0F) {
        std::cerr << "IMS speedway gearing stack is outside the target redline window\n";
        return 1;
    }
    for (std::size_t gear = 0; gear + 1 < vehicleConfig.gearRatios.size(); ++gear) {
        const float postShiftRpm =
            vehicleConfig.redlineRpm *
            vehicleConfig.gearRatios[gear + 1] / vehicleConfig.gearRatios[gear];
        if (!near(postShiftRpm, 9500.0F, 8.0F)) {
            std::cerr << "Geometric IMS gear split does not drop to 9500 RPM\n";
            return 1;
        }
        if (vehicleConfig.gearRatios[gear] <= vehicleConfig.gearRatios[gear + 1]) {
            std::cerr << "Gear ratios must descend monotonically\n";
            return 1;
        }
    }
    if (vehicleConfig.gearRatios[4] / vehicleConfig.gearRatios[3] < 0.78F ||
        vehicleConfig.gearRatios[5] / vehicleConfig.gearRatios[4] < 0.78F) {
        std::cerr << "IMS speedway top gears are not stacked closely enough\n";
        return 1;
    }

    sim::Vehicle vehicle(vehicleConfig);
    sim::InputActions input;
    input.throttle = 1.0F;

    sim::Vehicle launchVehicle(vehicleConfig);
    sim::InputActions launchInput;
    launchInput.throttle = 1.0F;
    for (int step = 0; step < stepsForSeconds(0.90F); ++step) {
        launchVehicle.step(launchInput, kPhysicsDt);
        if (launchVehicle.current().speedMps < kSecondGearLaunchUpshiftMinSpeedMps &&
            launchVehicle.current().gear > 2) {
            std::cerr << "Automatic launch shift skipped past second gear during rear wheelspin\n";
            return 1;
        }
    }

    sim::Vehicle manualShiftVehicle(vehicleConfig);
    manualShiftVehicle.setAutomaticShift(false);
    sim::InputActions manualShiftInput;
    manualShiftInput.shiftUp = true;
    manualShiftVehicle.step(manualShiftInput, kPhysicsDt);
    if (manualShiftVehicle.current().gear != 2) {
        std::cerr << "Manual upshift did not select second gear\n";
        return 1;
    }
    manualShiftInput.shiftUp = false;
    manualShiftVehicle.step(manualShiftInput, kPhysicsDt);
    manualShiftInput.shiftUp = true;
    manualShiftVehicle.step(manualShiftInput, kPhysicsDt);
    if (manualShiftVehicle.current().gear != 2) {
        std::cerr << "Shift cooldown allowed an immediate double upshift\n";
        return 1;
    }
    manualShiftInput.shiftUp = false;
    for (int step = 0; step < stepsForSeconds(0.22F); ++step) {
        manualShiftVehicle.step(manualShiftInput, kPhysicsDt);
    }
    manualShiftInput.shiftUp = true;
    manualShiftVehicle.step(manualShiftInput, kPhysicsDt);
    if (manualShiftVehicle.current().gear != 3) {
        std::cerr << "Shift cooldown did not release after the expected delay\n";
        return 1;
    }
    sim::Vehicle manualLimiterVehicle(vehicleConfig);
    manualLimiterVehicle.setAutomaticShift(false);
    sim::InputActions manualLimiterInput;
    manualLimiterInput.throttle = 1.0F;
    for (int step = 0; step < stepsForSeconds(4.0F); ++step) {
        manualLimiterVehicle.step(manualLimiterInput, kPhysicsDt);
    }
    if (manualLimiterVehicle.current().gear != 1 ||
        manualLimiterVehicle.current().rpm > vehicleConfig.redlineRpm ||
        manualLimiterVehicle.current().rpm < vehicleConfig.redlineRpm - 650.0F) {
        std::cerr << "Manual transmission allowed automatic shifts or failed to ride the limiter\n";
        return 1;
    }

    sim::Vehicle straightCamberVehicle(vehicleConfig);
    sim::InputActions straightCamberInput;
    straightCamberInput.throttle = 0.85F;
    float maximumStraightCamberYawRate = 0.0F;
    float maximumStraightCamberLateralSpeed = 0.0F;
    for (int step = 0; step < stepsForSeconds(1.5F); ++step) {
        straightCamberVehicle.step(straightCamberInput, kPhysicsDt);
        maximumStraightCamberYawRate =
            std::max(maximumStraightCamberYawRate, std::abs(straightCamberVehicle.current().yawRate));
        maximumStraightCamberLateralSpeed = std::max(
            maximumStraightCamberLateralSpeed,
            std::abs(straightCamberVehicle.current().lateralSpeedMps));
    }
    if (maximumStraightCamberYawRate > 0.06F ||
        maximumStraightCamberLateralSpeed > 0.35F) {
        std::cerr << "Static camber thrust should cancel left/right in straight-line running"
                  << " yawRate=" << maximumStraightCamberYawRate
                  << " lateralSpeed=" << maximumStraightCamberLateralSpeed << '\n';
        return 1;
    }

    for (int step = 0; step < stepsForSeconds(1.0F); ++step) {
        vehicle.step(input, kPhysicsDt);
    }
    if (vehicle.current().speedMps <= 0.0F) {
        std::cerr << "Vehicle did not accelerate\n";
        return 1;
    }
    if (vehicle.current().engineForceN <= 0.0F ||
        vehicle.current().rpm < vehicleConfig.idleRpm ||
        vehicle.current().downforceN <= 0.0F) {
        std::cerr << "Vehicle drivetrain/aero telemetry did not update\n";
        return 1;
    }
    if (vehicle.current().rearLeftWheelAngularVelocityRadPerSec <= 0.0F ||
        vehicle.current().rearRightWheelAngularVelocityRadPerSec <= 0.0F ||
        vehicle.current().frontRideHeightM <= 0.0F ||
        vehicle.current().rearRideHeightM <= 0.0F) {
        std::cerr << "Wheel-inertia or ride-height telemetry did not update\n";
        return 1;
    }

    const float speedBeforeBrake = vehicle.current().speedMps;
    input.throttle = 0.0F;
    input.brake = 1.0F;
    for (int step = 0; step < stepsForSeconds(1.0F); ++step) {
        vehicle.step(input, kPhysicsDt);
    }
    if (vehicle.current().speedMps >= speedBeforeBrake) {
        std::cerr << "Vehicle did not slow under braking\n";
        return 1;
    }
    if (vehicle.current().frontNormalLoadN <= vehicle.current().rearNormalLoadN * 0.6F) {
        std::cerr << "Braking/load-transfer telemetry looks wrong\n";
        return 1;
    }

    sim::Vehicle rigidBodyVehicle(vehicleConfig);
    sim::InputActions rigidBodyInput;
    rigidBodyInput.throttle = 1.0F;
    for (int step = 0; step < stepsForSeconds(1.5F); ++step) {
        rigidBodyVehicle.step(rigidBodyInput, kPhysicsDt);
    }
    rigidBodyInput.throttle = 0.35F;
    rigidBodyInput.steer = 0.70F;
    float maximumYawRate = 0.0F;
    float maximumFrontLateralForceN = 0.0F;
    for (int step = 0; step < stepsForSeconds(0.55F); ++step) {
        rigidBodyVehicle.step(rigidBodyInput, kPhysicsDt);
        maximumYawRate = std::max(maximumYawRate, std::abs(rigidBodyVehicle.current().yawRate));
        maximumFrontLateralForceN = std::max(
            maximumFrontLateralForceN,
            std::abs(rigidBodyVehicle.current().frontLateralForceN));
    }
    if (maximumYawRate <= 0.03F || maximumFrontLateralForceN <= 250.0F) {
        std::cerr << "Four-corner rigid-body tire forces did not create chassis yaw\n";
        return 1;
    }

    auto camberCornerFrontForce = [&](const sim::VehicleConfig& testConfig) {
        sim::Vehicle testVehicle(testConfig);
        sim::InputActions drive;
        drive.throttle = 1.0F;
        for (int step = 0; step < stepsForSeconds(1.8F); ++step) {
            testVehicle.step(drive, kPhysicsDt);
        }
        drive.throttle = 0.30F;
        drive.steer = 0.68F;
        float peakFrontForceN = 0.0F;
        for (int step = 0; step < stepsForSeconds(0.65F); ++step) {
            testVehicle.step(drive, kPhysicsDt);
            peakFrontForceN =
                std::max(peakFrontForceN, std::abs(testVehicle.current().frontLateralForceN));
        }
        return peakFrontForceN;
    };
    sim::VehicleConfig zeroCamberConfig = vehicleConfig;
    zeroCamberConfig.tireCamberStiffnessNPerRad = 0.0F;
    sim::VehicleConfig strongCamberConfig = vehicleConfig;
    strongCamberConfig.tireCamberStiffnessNPerRad = 5000.0F;
    strongCamberConfig.camberAngleFrontRadians = -0.120F;
    strongCamberConfig.camberAngleRearRadians = -0.060F;
    const float zeroCamberFrontForceN = camberCornerFrontForce(zeroCamberConfig);
    const float strongCamberFrontForceN = camberCornerFrontForce(strongCamberConfig);
    if (strongCamberFrontForceN <= zeroCamberFrontForceN * 1.003F) {
        std::cerr << "Camber thrust did not increase front lateral force"
                  << " zeroCamber=" << zeroCamberFrontForceN
                  << " strongCamber=" << strongCamberFrontForceN << '\n';
        return 1;
    }

    struct CornerResult {
        float maximumUsage = 0.0F;
        float maximumLateralG = 0.0F;
        float maximumFrontLoadSpreadN = 0.0F;
        float maximumTireTemperatureC = 0.0F;
        float minimumThermalGrip = 2.0F;
    };
    auto cornerResult = [&](float gripMultiplier) {
        sim::Vehicle testVehicle(vehicleConfig);
        sim::InputActions drive;
        drive.throttle = 1.0F;
        for (int step = 0; step < stepsForSeconds(2.0F); ++step) {
            testVehicle.step(drive, kPhysicsDt, gripMultiplier);
        }
        drive.throttle = 0.35F;
        drive.steer = 0.8F;
        CornerResult result;
        for (int step = 0; step < stepsForSeconds(0.75F); ++step) {
            testVehicle.step(drive, kPhysicsDt, gripMultiplier);
            result.maximumUsage = std::max(
                result.maximumUsage,
                std::max(testVehicle.current().frontTireUsage, testVehicle.current().rearTireUsage));
            result.maximumLateralG =
                std::max(result.maximumLateralG, std::abs(testVehicle.current().lateralG));
            result.maximumFrontLoadSpreadN = std::max(
                result.maximumFrontLoadSpreadN,
                std::abs(
                    testVehicle.current().frontLeftNormalLoadN -
                    testVehicle.current().frontRightNormalLoadN));
            result.maximumTireTemperatureC = std::max(
                result.maximumTireTemperatureC,
                std::max({
                    testVehicle.current().frontLeftTireTemperatureC,
                    testVehicle.current().frontRightTireTemperatureC,
                    testVehicle.current().rearLeftTireTemperatureC,
                    testVehicle.current().rearRightTireTemperatureC,
                }));
            result.minimumThermalGrip = std::min(
                result.minimumThermalGrip,
                std::min({
                    testVehicle.current().frontLeftThermalGrip,
                    testVehicle.current().frontRightThermalGrip,
                    testVehicle.current().rearLeftThermalGrip,
                    testVehicle.current().rearRightThermalGrip,
                }));
        }
        return result;
    };
    const CornerResult asphaltCorner = cornerResult(1.0F);
    const CornerResult grassCorner = cornerResult(0.45F);
    if (grassCorner.maximumLateralG >= asphaltCorner.maximumLateralG * 0.75F ||
        grassCorner.maximumUsage <= 0.9F) {
        std::cerr << "Low-grip surface did not reduce available cornering force\n";
        return 1;
    }
    if (asphaltCorner.maximumFrontLoadSpreadN <= 50.0F) {
        std::cerr << "Four-wheel load transfer telemetry did not update\n";
        return 1;
    }
    if (asphaltCorner.maximumTireTemperatureC <= 31.0F ||
        asphaltCorner.minimumThermalGrip < 0.60F ||
        asphaltCorner.minimumThermalGrip > 1.08F) {
        std::cerr << "Dynamic tire temperature/grip telemetry did not update\n";
        return 1;
    }

    sim::VehicleConfig linearLoadConfig = vehicleConfig;
    linearLoadConfig.tireLoadSensitivityCoeff = 0.0F;
    auto highLoadCornerResult = [&](const sim::VehicleConfig& testConfig) {
        sim::Vehicle testVehicle(testConfig);
        sim::InputActions drive;
        drive.throttle = 1.0F;
        for (int step = 0; step < stepsForSeconds(2.2F); ++step) {
            testVehicle.step(drive, kPhysicsDt);
        }
        drive.throttle = 0.25F;
        drive.steer = 0.95F;
        CornerResult result;
        for (int step = 0; step < stepsForSeconds(0.85F); ++step) {
            testVehicle.step(drive, kPhysicsDt);
            result.maximumUsage = std::max(
                result.maximumUsage,
                std::max(testVehicle.current().frontTireUsage, testVehicle.current().rearTireUsage));
            result.maximumLateralG =
                std::max(result.maximumLateralG, std::abs(testVehicle.current().lateralG));
        }
        return result;
    };
    const CornerResult linearLoadCorner = highLoadCornerResult(linearLoadConfig);
    const CornerResult degressiveLoadCorner = highLoadCornerResult(vehicleConfig);
    if (degressiveLoadCorner.maximumLateralG >= linearLoadCorner.maximumLateralG * 0.92F) {
        std::cerr << "Degressive load sensitivity did not reduce high-load cornering efficiency"
                  << " linearG=" << linearLoadCorner.maximumLateralG
                  << " degressiveG=" << degressiveLoadCorner.maximumLateralG << '\n';
        return 1;
    }
    if (std::abs(vehicle.current().chassisPitchRadians) <= 0.0001F &&
        std::abs(vehicle.current().chassisHeaveM) <= 0.0001F) {
        std::cerr << "Sprung chassis telemetry did not move under acceleration/braking\n";
        return 1;
    }

    vehicle.reset();
    if (!near(vehicle.current().speedMps, 0.0F)) {
        std::cerr << "Vehicle reset failed\n";
        return 1;
    }

    return 0;
}
