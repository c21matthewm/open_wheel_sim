#pragma once

#include <array>

namespace sim {

class ConfigFile;

struct VehicleConfig {
    float massKg = 733.0F;
    float yawInertiaKgM2 = 880.0F;
    float pitchInertiaKgM2 = 760.0F;
    float rollInertiaKgM2 = 320.0F;
    float wheelbaseM = 3.035F;
    float trackWidthM = 1.943F;
    float centerOfMassHeightM = 0.30F;
    float frontWeightFraction = 0.44F;
    float unsprungMassPerWheelKg = 16.5F;
    float maxRoadWheelAngleRadians = 0.383972F;
    float highSpeedSteerScale = 0.22F;
    float steerSpeedThresholdMps = 75.0F;
    float frictionCoefficient = 1.60F;
    float frontCorneringStiffness = 90000.0F;
    float rearCorneringStiffness = 105000.0F;
    float lateralPeakSlipAngleRadians = 0.118682F;
    float longitudinalPeakSlipRatio = 0.105F;
    float tireCurveShapeFactor = 1.38F;
    float tireCurveCurvatureFactor = 0.72F;
    float tirePostPeakFalloff = 0.18F;
    float lateralLoadTransferGripLoss = 0.18F;
    float tireLoadSensitivityCoeff = 0.10F;
    float tireLoadSensitivityMinEfficiency = 0.65F;
    float tireLoadReferenceNormalN = 1500.0F;
    float tireCamberStiffnessNPerRad = 1000.0F;
    float camberAngleFrontRadians = -0.052F;
    float camberAngleRearRadians = -0.017F;
    float speedwayCamberAngleFrontRadians = -0.052F;
    float speedwayCamberAngleRearRadians = -0.017F;
    float roadCourseCamberAngleFrontRadians = -0.070F;
    float roadCourseCamberAngleRearRadians = -0.030F;
    float tirePneumaticTrailMaxM = 0.018F;
    float tireMechanicalTrailM = 0.010F;
    float tireRelaxationLengthM = 0.12F;
    float tireLongitudinalStiffness = 78000.0F;
    float frontRollStiffnessFraction = 0.52F;
    float frontSpringRateNPerM = 100000.0F;
    float rearSpringRateNPerM = 110000.0F;
    float frontDamperNPerMps = 8800.0F;
    float rearDamperNPerMps = 9800.0F;
    float frontAntiRollBarNPerM = 38000.0F;
    float rearAntiRollBarNPerM = 34000.0F;
    float maxSuspensionCompressionM = 0.085F;
    float maxSuspensionDroopM = 0.055F;
    float bumpStopRateNPerM = 450000.0F;
    float bumpStopDampingNPerMps = 11000.0F;
    float frontStaticRideHeightM = 0.045F;
    float rearStaticRideHeightM = 0.060F;
    float tireVerticalStiffnessNPerM = 195000.0F;
    float tireVerticalDampingNPerMps = 4500.0F;
    float engineTorqueNm = 475.0F;
    float engineBrakingN = 1050.0F;
    float drivetrainEfficiency = 0.93F;
    float driveFrontFraction = 0.0F;
    float differentialLoadBias = 0.35F;
    float wheelRadiusM = 0.343F;
    float wheelInertiaKgM2 = 1.25F;
    float finalDriveRatio = 3.65F;
    std::array<float, 6> gearRatios{3.55345F, 2.81315F, 2.22708F, 1.76310F, 1.39579F, 1.105F};
    float idleRpm = 3000.0F;
    float redlineRpm = 12000.0F;
    float shiftUpRpm = 11500.0F;
    float shiftDownRpm = 5500.0F;
    bool automaticShift = true;
    float brakeForceN = 22000.0F;
    float brakeBias = 0.58F;
    float brakeGamma = 1.10F;
    float aeroDragNPerMps2 = 0.72F;
    float downforceNPerMps2 = 1.95F;
    float frontDownforceFraction = 0.43F;
    float aeroReferenceFrontRideHeightM = 0.048F;
    float aeroReferenceRearRideHeightM = 0.062F;
    float groundEffectRideHeightScaleM = 0.028F;
    float maxGroundEffectMultiplier = 2.90F;
    float aeroStallRideHeightM = 0.016F;
    float aeroStallDownforceMultiplier = 0.45F;
    float aeroCopShiftPerMeter = 1.95F;
    float aeroRideHeightBalanceSensitivity = 0.22F;
    float aeroBrakeCopShift = 0.035F;
    float aeroStallCopShift = 0.060F;
    float aeroInstantLoadFraction = 0.16F;
    float minFrontDownforceFraction = 0.30F;
    float maxFrontDownforceFraction = 0.60F;
    float rollingResistanceN = 160.0F;

    void load(const ConfigFile& config);
};

}  // namespace sim
