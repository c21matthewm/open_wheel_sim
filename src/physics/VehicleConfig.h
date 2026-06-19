#pragma once

#include <array>
#include <cstddef>

namespace sim {

class ConfigFile;

struct AeroPresetConfig {
    float downforceNPerMps2 = 3.0F;
    float dragNPerMps2 = 0.55F;
    float frontDownforceFraction = 0.37F;
    float brakeCopShift = 0.020F;
    float stallRideHeightM = 0.040F;
    float stallDownforceMultiplier = 0.60F;
};

struct TorqueCurveKnot {
    float rpmNorm = 0.0F;
    float torqueNorm = 0.0F;
};

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
    float maxRoadWheelAngleRadians = 0.314159F;
    float highSpeedSteerScale = 0.45F;
    float steerSpeedThresholdMps = 90.0F;
    float frictionCoefficient = 1.60F;
    float frontCorneringStiffness = 350000.0F;
    float rearCorneringStiffness = 400000.0F;
    float lateralPeakSlipAngleRadians = 0.118682F;
    float longitudinalPeakSlipRatio = 0.105F;
    float tireCurveShapeFactor = 1.38F;
    float tireCurveCurvatureFactor = 0.72F;
    float tirePostPeakFalloff = 0.20F;
    float lateralLoadTransferGripLoss = 0.18F;
    float tireLoadSensitivityCoeff = 0.10F;
    float tireLoadSensitivityMinEfficiency = 0.65F;
    float tireLoadReferenceNormalN = 1500.0F;
    float tireCamberStiffnessNPerRad = 15000.0F;
    float camberAngleFrontRadians = -0.052F;
    float camberAngleRearRadians = -0.017F;
    float speedwayCamberAngleFrontRadians = -0.052F;
    float speedwayCamberAngleRearRadians = -0.017F;
    float roadCourseCamberAngleFrontRadians = -0.070F;
    float roadCourseCamberAngleRearRadians = -0.030F;
    float tirePneumaticTrailMaxM = 0.018F;
    float tireMechanicalTrailM = 0.010F;
    float tireRelaxationLengthBaseM = 0.40F;
    float tireRelaxationLengthMinM = 0.15F;
    float tireRelaxationLengthMaxM = 0.90F;
    float tireLongitudinalStiffness = 450000.0F;
    float tireStiffnessSpeedSoftening = 0.08F;
    float tireStiffnessSpeedReferenceRadPerSec = 80.0F;
    float tirePacejkaMinStiffnessFactor = 3.0F;
    float tirePacejkaPeakForceTarget = 0.995F;
    float tirePacejkaMaxStiffnessFactor = 9.0F;
    float tireLongitudinalGripFraction = 0.93F;
    float tireThermalOptimalC = 95.0F;
    float tireThermalWindowC = 35.0F;
    float tireThermalGripMin = 0.72F;
    float tireWearMinGrip = 0.84F;
    float tireWearSlidingRatePerWork = 0.000020F;
    float tireWearWheelspinRatePerWork = 0.000002F;
    float frontRollStiffnessFraction = 0.52F;
    float frontSpringRateNPerM = 100000.0F;
    float rearSpringRateNPerM = 110000.0F;
    float frontDamperNPerMps = 8800.0F;
    float rearDamperNPerMps = 9800.0F;
    float frontDamperBumpNPerMps = 6500.0F;
    float frontDamperReboundNPerMps = 9500.0F;
    float rearDamperBumpNPerMps = 7200.0F;
    float rearDamperReboundNPerMps = 11000.0F;
    float longitudinalLoadTransferTauS = 0.08F;
    float frontCamberGainRadiansPerM = 0.209440F;
    float rearCamberGainRadiansPerM = 0.139626F;
    float frontAntiRollBarNPerM = 38000.0F;
    float rearAntiRollBarNPerM = 34000.0F;
    float frontAntiRollBarNmPerRad = 0.0F;
    float rearAntiRollBarNmPerRad = 0.0F;
    bool useDynamicRollStiffnessFraction = false;
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
    bool useLimitedSlipDifferential = true;
    float lsdPreloadNm = 40.0F;
    float lsdRampFactor = 0.25F;
    float lsdSensitivity = 0.04F;
    float wheelRadiusM = 0.343F;
    float wheelInertiaKgM2 = 1.25F;
    float finalDriveRatio = 3.65F;
    std::array<float, 6> gearRatios{3.55345F, 2.81315F, 2.22708F, 1.76310F, 1.39579F, 1.105F};
    float idleRpm = 3000.0F;
    float redlineRpm = 12000.0F;
    float shiftUpRpm = 11500.0F;
    float shiftDownRpm = 5500.0F;
    float limiterStartMarginRpm = 160.0F;
    float limiterFullMarginRpm = 20.0F;
    std::array<TorqueCurveKnot, 8> torqueCurveKnots{{
        {0.00F, 0.30F},
        {0.45F, 0.80F},
        {0.75F, 1.00F},
        {0.90F, 0.97F},
        {0.97F, 0.95F},
        {0.995F, 0.95F},
        {1.00F, 0.00F},
    }};
    std::size_t torqueCurveKnotCount = 7;
    bool automaticShift = true;
    float fuelCapacityGallons = 18.5F;
    float fuelInitialGallons = 18.5F;
    float fuelBurnGallonsPerSecondAtRedline = 0.0140F;
    float fuelAverageBurnResponseHz = 0.12F;
    float fuelIdleLoadFactor = 0.14F;
    float fuelIdleRpmFactor = 0.35F;
    float engineMapLeanFuelMultiplier = 0.88F;
    float engineMapStandardFuelMultiplier = 1.0F;
    float engineMapRichFuelMultiplier = 1.15F;
    float engineMapLeanTorqueMultiplier = 0.96F;
    float engineMapStandardTorqueMultiplier = 1.0F;
    float engineMapRichTorqueMultiplier = 1.035F;
    float brakeForceN = 22000.0F;
    float brakeBias = 0.58F;
    float brakeGamma = 1.10F;
    float aeroDragNPerMps2 = 0.34F;
    float downforceNPerMps2 = 3.0F;
    float frontDownforceFraction = 0.37F;
    float aeroReferenceFrontRideHeightM = 0.048F;
    float aeroReferenceRearRideHeightM = 0.062F;
    float groundEffectRideHeightScaleM = 0.028F;
    float maxGroundEffectMultiplier = 2.90F;
    float aeroStallRideHeightM = 0.040F;
    float aeroStallDownforceMultiplier = 0.60F;
    float aeroCopShiftPerMeter = 1.95F;
    float aeroRideHeightBalanceSensitivity = 0.22F;
    float aeroBrakeCopShift = 0.020F;
    float aeroStallCopShift = 0.060F;
    float aeroInstantLoadFraction = 0.16F;
    float aeroYawDampingNmPerRadS = 400.0F;
    float aeroYawDampingReferenceSpeedMps = 60.0F;
    float aeroYawDampingRearSlideMinScale = 0.20F;
    float aeroYawDampingRearSlideFullSaturation = 1.45F;
    float minFrontDownforceFraction = 0.30F;
    float maxFrontDownforceFraction = 0.60F;
    AeroPresetConfig speedwayAeroPreset{3.0F, 0.34F, 0.37F, 0.020F, 0.040F, 0.60F};
    AeroPresetConfig roadCourseAeroPreset{4.5F, 0.82F, 0.42F, 0.035F, 0.045F, 0.55F};
    float rollingResistanceN = 160.0F;
    float frontWingSetting = 0.0F;
    float rearWingSetting = 0.0F;
    float wingSettingMin = -3.0F;
    float wingSettingMax = 3.0F;
    float frontWingAeroBalancePerStep = 0.012F;
    float frontWingCorneringStiffnessPerStep = 0.018F;
    float rearWingDownforcePerStep = 0.025F;
    float rearWingDragPerStep = 0.030F;
    float rearWingCorneringStiffnessPerStep = 0.016F;

    void load(const ConfigFile& config);
};

}  // namespace sim
