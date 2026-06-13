#pragma once

#include "input/InputActions.h"
#include "physics/VehicleConfig.h"

namespace sim {

struct VehicleState {
    float positionX = 0.0F;
    float positionZ = 0.0F;
    float velocityX = 0.0F;
    float velocityZ = 0.0F;
    float yawRadians = 0.0F;
    float yawRate = 0.0F;
    float chassisHeaveM = 0.0F;
    float chassisHeaveVelocityMps = 0.0F;
    float chassisPitchRadians = 0.0F;
    float chassisPitchRate = 0.0F;
    float chassisRollRadians = 0.0F;
    float chassisRollRate = 0.0F;
    float frontRideHeightM = 0.0F;
    float rearRideHeightM = 0.0F;
    float aeroCenterOfPressure = 0.45F;
    float floorStrikeIntensity = 0.0F;
    float speedMps = 0.0F;
    float longitudinalSpeedMps = 0.0F;
    float lateralSpeedMps = 0.0F;
    float steeringAngleRadians = 0.0F;
    float wheelRotationRadians = 0.0F;
    float frontLeftWheelRotationRadians = 0.0F;
    float frontRightWheelRotationRadians = 0.0F;
    float rearLeftWheelRotationRadians = 0.0F;
    float rearRightWheelRotationRadians = 0.0F;
    float frontLeftWheelAngularVelocityRadPerSec = 0.0F;
    float frontRightWheelAngularVelocityRadPerSec = 0.0F;
    float rearLeftWheelAngularVelocityRadPerSec = 0.0F;
    float rearRightWheelAngularVelocityRadPerSec = 0.0F;
    float frontLeftWheelHubTravelM = 0.0F;
    float frontRightWheelHubTravelM = 0.0F;
    float rearLeftWheelHubTravelM = 0.0F;
    float rearRightWheelHubTravelM = 0.0F;
    float frontLeftWheelHubVelocityMps = 0.0F;
    float frontRightWheelHubVelocityMps = 0.0F;
    float rearLeftWheelHubVelocityMps = 0.0F;
    float rearRightWheelHubVelocityMps = 0.0F;
    float frontLeftSuspensionTravelM = 0.0F;
    float frontRightSuspensionTravelM = 0.0F;
    float rearLeftSuspensionTravelM = 0.0F;
    float rearRightSuspensionTravelM = 0.0F;
    float frontSlipAngleRadians = 0.0F;
    float rearSlipAngleRadians = 0.0F;
    float frontLeftSlipAngleRadians = 0.0F;
    float frontRightSlipAngleRadians = 0.0F;
    float rearLeftSlipAngleRadians = 0.0F;
    float rearRightSlipAngleRadians = 0.0F;
    float frontLongitudinalSlip = 0.0F;
    float rearLongitudinalSlip = 0.0F;
    float frontLeftLongitudinalSlip = 0.0F;
    float frontRightLongitudinalSlip = 0.0F;
    float rearLeftLongitudinalSlip = 0.0F;
    float rearRightLongitudinalSlip = 0.0F;
    float frontNormalLoadN = 0.0F;
    float rearNormalLoadN = 0.0F;
    float frontLeftNormalLoadN = 0.0F;
    float frontRightNormalLoadN = 0.0F;
    float rearLeftNormalLoadN = 0.0F;
    float rearRightNormalLoadN = 0.0F;
    float frontLateralForceN = 0.0F;
    float rearLateralForceN = 0.0F;
    float frontLeftLateralForceN = 0.0F;
    float frontRightLateralForceN = 0.0F;
    float rearLeftLateralForceN = 0.0F;
    float rearRightLateralForceN = 0.0F;
    float frontLongitudinalForceN = 0.0F;
    float rearLongitudinalForceN = 0.0F;
    float frontTireUsage = 0.0F;
    float rearTireUsage = 0.0F;
    float frontLeftTireUsage = 0.0F;
    float frontRightTireUsage = 0.0F;
    float rearLeftTireUsage = 0.0F;
    float rearRightTireUsage = 0.0F;
    float frontLeftTireTemperatureC = 0.0F;
    float frontRightTireTemperatureC = 0.0F;
    float rearLeftTireTemperatureC = 0.0F;
    float rearRightTireTemperatureC = 0.0F;
    float frontLeftThermalGrip = 1.0F;
    float frontRightThermalGrip = 1.0F;
    float rearLeftThermalGrip = 1.0F;
    float rearRightThermalGrip = 1.0F;
    float engineForceN = 0.0F;
    float brakeForceN = 0.0F;
    float dragForceN = 0.0F;
    float downforceN = 0.0F;
    float brakeBias = 0.0F;
    float lateralG = 0.0F;
    float longitudinalG = 0.0F;
    float rpm = 1200.0F;
    int gear = 1;
};

class Vehicle {
public:
    explicit Vehicle(VehicleConfig config);

    void reset();
    void reset(float positionX, float positionZ, float yawRadians);
    void step(
        const InputActions& input,
        float deltaSeconds,
        float gripMultiplier = 1.0F,
        float longitudinalGripMultiplier = -1.0F,
        float resistanceMultiplier = 1.0F,
        float bankRadians = 0.0F,
        float trackTangentX = 0.0F,
        float trackTangentZ = 1.0F);
    void resolveBoundary(
        float outwardNormalX,
        float outwardNormalZ,
        float penetrationM,
        float restitution,
        float contactOffsetX = 0.0F,
        float contactOffsetZ = 0.0F);
    void applyVelocityDamping(float dampingPerSecond, float deltaSeconds);
    void setBrakeBias(float brakeBias);
    void setAutomaticShift(bool automaticShift);
    void setAeroPreset(int preset);
    [[nodiscard]] int aeroPreset() const { return aeroPreset_; }
    [[nodiscard]] const char* aeroPresetName() const;
    [[nodiscard]] VehicleState interpolated(float alpha) const;
    [[nodiscard]] const VehicleState& current() const { return current_; }
    [[nodiscard]] float brakeBias() const { return config_.brakeBias; }
    [[nodiscard]] bool automaticShift() const { return config_.automaticShift; }
    [[nodiscard]] float collisionHalfLengthM() const;
    [[nodiscard]] float collisionHalfWidthM() const;

private:
    [[nodiscard]] float gearRatioFor(int gear) const;
    [[nodiscard]] float engineRpmForSpeed(float absoluteLongitudinalSpeed) const;
    [[nodiscard]] float engineRpmForDrivenWheels() const;
    [[nodiscard]] float engineTorqueAt(float rpm, float throttle) const;
    void updateGear(float engineRpm, const InputActions& input);
    void refreshSpeedTelemetry();
    void resetDynamicState();

    VehicleConfig config_;
    VehicleState previous_;
    VehicleState current_;
    int currentGear_ = 1;
    bool previousShiftUp_ = false;
    bool previousShiftDown_ = false;
    float shiftCooldownSeconds_ = 0.0F;
    float rpmLimiterPhase_ = 0.0F;
    float rpmLimiterCut_ = 0.0F;
    int aeroPreset_ = 0;
};

}  // namespace sim
