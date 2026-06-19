#pragma once

#include <array>

#include "input/InputActions.h"
#include "physics/VehicleConfig.h"

namespace sim {

struct WheelSurfaceContact {
    float lateralGripMultiplier = 1.0F;
    float longitudinalGripMultiplier = 1.0F;
    float resistanceMultiplier = 1.0F;
    float bankRadians = 0.0F;
    float roadPitchRadians = 0.0F;
    float tangentX = 0.0F;
    float tangentZ = 1.0F;
    float roadHeightM = 0.0F;
    float roadNormalX = 0.0F;
    float roadNormalY = 1.0F;
    float roadNormalZ = 0.0F;
    float surfaceRoughnessM = 0.0F;
    float curbHeightM = 0.0F;
    float curbPhase = 0.0F;
    float contactHeightFilterTauS = 0.0F;
};

struct WheelContactPatch {
    float x = 0.0F;
    float z = 0.0F;
};

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
    float frontLeftRoadHeightM = 0.0F;
    float frontRightRoadHeightM = 0.0F;
    float rearLeftRoadHeightM = 0.0F;
    float rearRightRoadHeightM = 0.0F;
    float frontLeftRoadHeightVelocityMps = 0.0F;
    float frontRightRoadHeightVelocityMps = 0.0F;
    float rearLeftRoadHeightVelocityMps = 0.0F;
    float rearRightRoadHeightVelocityMps = 0.0F;
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
    std::array<float, 4> relaxedSlipRatio{};
    float frontNormalLoadN = 0.0F;
    float rearNormalLoadN = 0.0F;
    float frontLeftNormalLoadN = 0.0F;
    float frontRightNormalLoadN = 0.0F;
    float rearLeftNormalLoadN = 0.0F;
    float rearRightNormalLoadN = 0.0F;
    float filtered_long_load_transfer_n = 0.0F;
    float frontRollStiffnessNmPerRad = 0.0F;
    float rearRollStiffnessNmPerRad = 0.0F;
    float frontRollStiffnessFraction = 0.0F;
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
    float frontLeftTireWear = 1.0F;
    float frontRightTireWear = 1.0F;
    float rearLeftTireWear = 1.0F;
    float rearRightTireWear = 1.0F;
    float frontLeftCamberRadians = 0.0F;
    float frontRightCamberRadians = 0.0F;
    float rearLeftCamberRadians = 0.0F;
    float rearRightCamberRadians = 0.0F;
    float frontPneumaticTrailM = 0.0F;
    float rearPneumaticTrailM = 0.0F;
    float frontLeftPneumaticTrailM = 0.0F;
    float frontRightPneumaticTrailM = 0.0F;
    float rearLeftPneumaticTrailM = 0.0F;
    float rearRightPneumaticTrailM = 0.0F;
    float frontLeftAligningTrailM = 0.0F;
    float frontRightAligningTrailM = 0.0F;
    float rearLeftAligningTrailM = 0.0F;
    float rearRightAligningTrailM = 0.0F;
    float frontLeftRelaxationLengthM = 0.0F;
    float frontRightRelaxationLengthM = 0.0F;
    float rearLeftRelaxationLengthM = 0.0F;
    float rearRightRelaxationLengthM = 0.0F;
    float engineForceN = 0.0F;
    float brakeForceN = 0.0F;
    float dragForceN = 0.0F;
    float downforceN = 0.0F;
    float brakeBias = 0.0F;
    float frontWingSetting = 0.0F;
    float rearWingSetting = 0.0F;
    float fuelGallons = 0.0F;
    float fuelBurnRateGps = 0.0F;
    float fuelAverageBurnRateGps = 0.0F;
    int engineMap = 1;
    float lateralG = 0.0F;
    float longitudinalG = 0.0F;
    float rpm = 1200.0F;
    float rpmArcStartRpm = 9000.0F;
    float shiftUpRpm = 11500.0F;
    float redlineRpm = 12000.0F;
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
        float roadPitchRadians = 0.0F,
        float trackTangentX = 0.0F,
        float trackTangentZ = 1.0F,
        const std::array<WheelSurfaceContact, 4>* wheelSurfaceContacts = nullptr);
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
    void cycleEngineMap();
    void setEngineMap(int engineMap);
    void setFrontWingSetting(float setting);
    void setRearWingSetting(float setting);
    void adjustFrontWingSetting(float delta);
    void adjustRearWingSetting(float delta);
    void setAeroPreset(int preset);
    [[nodiscard]] int aeroPreset() const { return aeroPreset_; }
    [[nodiscard]] const char* aeroPresetName() const;
    [[nodiscard]] int engineMap() const { return engineMap_; }
    [[nodiscard]] const char* engineMapName() const;
    [[nodiscard]] VehicleState interpolated(float alpha) const;
    [[nodiscard]] const VehicleState& current() const { return current_; }
    [[nodiscard]] float brakeBias() const { return config_.brakeBias; }
    [[nodiscard]] bool automaticShift() const { return config_.automaticShift; }
    [[nodiscard]] float frontWingSetting() const { return config_.frontWingSetting; }
    [[nodiscard]] float rearWingSetting() const { return config_.rearWingSetting; }
    [[nodiscard]] std::array<WheelContactPatch, 4> wheelContactPatchesWorld() const;
    [[nodiscard]] float collisionHalfLengthM() const;
    [[nodiscard]] float collisionHalfWidthM() const;
    [[nodiscard]] static float aeroYawAlignmentFactor(
        float yawRadians,
        float velocityX,
        float velocityZ);

private:
    [[nodiscard]] float gearRatioFor(int gear) const;
    [[nodiscard]] float rpmArcStartForGear(int gear) const;
    [[nodiscard]] float engineRpmForSpeed(float absoluteLongitudinalSpeed) const;
    [[nodiscard]] float engineRpmForDrivenWheels() const;
    [[nodiscard]] float engineTorqueAt(float rpm, float throttle) const;
    [[nodiscard]] float engineMapTorqueMultiplier() const;
    [[nodiscard]] float engineMapFuelMultiplier() const;
    [[nodiscard]] float fuelBurnRate(float rpm, float throttle) const;
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
    int engineMap_ = 1;
};

}  // namespace sim
