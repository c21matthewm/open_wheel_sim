#include "physics/VehicleConfig.h"

#include <algorithm>
#include <numbers>
#include <string>

#include "config/ConfigFile.h"

namespace sim {

void VehicleConfig::load(const ConfigFile& config) {
    massKg = std::max(100.0F, config.getFloat("body.mass_kg", massKg));
    yawInertiaKgM2 = std::max(100.0F, config.getFloat("body.yaw_inertia_kg_m2", yawInertiaKgM2));
    pitchInertiaKgM2 =
        std::max(100.0F, config.getFloat("body.pitch_inertia_kg_m2", pitchInertiaKgM2));
    rollInertiaKgM2 =
        std::max(50.0F, config.getFloat("body.roll_inertia_kg_m2", rollInertiaKgM2));
    wheelbaseM = std::max(1.0F, config.getFloat("body.wheelbase_m", wheelbaseM));
    trackWidthM = std::max(0.5F, config.getFloat("body.track_width_m", trackWidthM));
    centerOfMassHeightM =
        std::clamp(config.getFloat("body.center_of_mass_height_m", centerOfMassHeightM), 0.05F, 1.5F);
    frontWeightFraction =
        std::clamp(config.getFloat("body.front_weight_fraction", frontWeightFraction), 0.2F, 0.8F);
    unsprungMassPerWheelKg =
        std::clamp(config.getFloat("body.unsprung_mass_per_wheel_kg", unsprungMassPerWheelKg), 5.0F, 60.0F);
    const float steeringDegrees =
        std::clamp(config.getFloat("steering.max_road_wheel_angle_deg", 20.0F), 1.0F, 45.0F);
    maxRoadWheelAngleRadians = steeringDegrees * std::numbers::pi_v<float> / 180.0F;
    highSpeedSteerScale =
        std::clamp(config.getFloat("steering.high_speed_steer_scale", highSpeedSteerScale), 0.02F, 1.0F);
    steerSpeedThresholdMps =
        std::max(5.0F, config.getFloat("steering.steer_speed_threshold_mps", steerSpeedThresholdMps));
    frictionCoefficient =
        std::max(0.1F, config.getFloat("tires.friction_coefficient", frictionCoefficient));
    frontCorneringStiffness = std::max(
        1000.0F,
        config.getFloat("tires.front_cornering_stiffness_n_per_rad", frontCorneringStiffness));
    rearCorneringStiffness = std::max(
        1000.0F,
        config.getFloat("tires.rear_cornering_stiffness_n_per_rad", rearCorneringStiffness));
    const float lateralPeakSlipDegrees = std::clamp(
        config.getFloat(
            "tires.lateral_peak_slip_angle_deg",
            lateralPeakSlipAngleRadians * 180.0F / std::numbers::pi_v<float>),
        2.0F,
        18.0F);
    lateralPeakSlipAngleRadians = lateralPeakSlipDegrees * std::numbers::pi_v<float> / 180.0F;
    longitudinalPeakSlipRatio = std::clamp(
        config.getFloat("tires.longitudinal_peak_slip_ratio", longitudinalPeakSlipRatio),
        0.02F,
        0.35F);
    tireCurveShapeFactor = std::clamp(
        config.getFloat("tires.curve_shape_factor", tireCurveShapeFactor),
        0.80F,
        2.20F);
    tireCurveCurvatureFactor = std::clamp(
        config.getFloat("tires.curve_curvature_factor", tireCurveCurvatureFactor),
        -1.0F,
        1.0F);
    tirePostPeakFalloff = std::clamp(
        config.getFloat("tires.post_peak_falloff", tirePostPeakFalloff),
        0.0F,
        0.45F);
    lateralLoadTransferGripLoss = std::clamp(
        config.getFloat("tires.lateral_load_transfer_grip_loss", lateralLoadTransferGripLoss),
        0.0F,
        0.75F);
    const float legacyLoadSensitivity =
        config.getFloat("tires.load_sensitivity", tireLoadSensitivityCoeff);
    tireLoadSensitivityCoeff = std::clamp(
        config.getFloat("tires.load_sensitivity_coeff", legacyLoadSensitivity),
        0.0F,
        0.35F);
    tireLoadSensitivityMinEfficiency = std::clamp(
        config.getFloat("tires.load_sensitivity_min_efficiency", tireLoadSensitivityMinEfficiency),
        0.35F,
        1.0F);
    tireLoadReferenceNormalN = std::clamp(
        config.getFloat("tires.load_reference_normal_n", tireLoadReferenceNormalN),
        250.0F,
        8000.0F);
    tireCamberStiffnessNPerRad = std::clamp(
        config.getFloat("tires.camber_stiffness_n_per_rad", tireCamberStiffnessNPerRad),
        0.0F,
        5000.0F);
    speedwayCamberAngleFrontRadians = std::clamp(
        config.getFloat("tires.camber_angle_front_rad", speedwayCamberAngleFrontRadians),
        -0.18F,
        0.18F);
    speedwayCamberAngleRearRadians = std::clamp(
        config.getFloat("tires.camber_angle_rear_rad", speedwayCamberAngleRearRadians),
        -0.18F,
        0.18F);
    roadCourseCamberAngleFrontRadians = std::clamp(
        config.getFloat("tires.road_course_camber_angle_front_rad", roadCourseCamberAngleFrontRadians),
        -0.18F,
        0.18F);
    roadCourseCamberAngleRearRadians = std::clamp(
        config.getFloat("tires.road_course_camber_angle_rear_rad", roadCourseCamberAngleRearRadians),
        -0.18F,
        0.18F);
    camberAngleFrontRadians = speedwayCamberAngleFrontRadians;
    camberAngleRearRadians = speedwayCamberAngleRearRadians;
    tirePneumaticTrailMaxM = std::clamp(
        config.getFloat("tires.pneumatic_trail_max_m", tirePneumaticTrailMaxM),
        0.0F,
        0.12F);
    tireMechanicalTrailM = std::clamp(
        config.getFloat("tires.mechanical_trail_m", tireMechanicalTrailM),
        0.0F,
        0.12F);
    const float legacyRelaxationLength =
        config.getFloat("tires.relaxation_length_m", tireRelaxationLengthBaseM);
    tireRelaxationLengthBaseM = std::clamp(
        config.getFloat("tires.relaxation_length_base_m", legacyRelaxationLength),
        0.03F,
        2.0F);
    tireRelaxationLengthMinM = std::clamp(
        config.getFloat("tires.relaxation_length_min_m", tireRelaxationLengthMinM),
        0.02F,
        tireRelaxationLengthBaseM);
    tireRelaxationLengthMaxM = std::clamp(
        config.getFloat("tires.relaxation_length_max_m", tireRelaxationLengthMaxM),
        tireRelaxationLengthMinM,
        3.0F);
    tireLongitudinalStiffness = std::max(
        1000.0F,
        config.getFloat("tires.longitudinal_stiffness_n", tireLongitudinalStiffness));
    frontRollStiffnessFraction = std::clamp(
        config.getFloat("tires.front_roll_stiffness_fraction", frontRollStiffnessFraction),
        0.1F,
        0.9F);
    frontSpringRateNPerM = std::max(
        1000.0F,
        config.getFloat("suspension.front_spring_rate_n_per_m", frontSpringRateNPerM));
    rearSpringRateNPerM = std::max(
        1000.0F,
        config.getFloat("suspension.rear_spring_rate_n_per_m", rearSpringRateNPerM));
    frontDamperNPerMps = std::max(
        0.0F,
        config.getFloat("suspension.front_damper_n_per_mps", frontDamperNPerMps));
    rearDamperNPerMps =
        std::max(0.0F, config.getFloat("suspension.rear_damper_n_per_mps", rearDamperNPerMps));
    frontAntiRollBarNPerM = std::max(
        0.0F,
        config.getFloat("suspension.front_anti_roll_bar_n_per_m", frontAntiRollBarNPerM));
    rearAntiRollBarNPerM = std::max(
        0.0F,
        config.getFloat("suspension.rear_anti_roll_bar_n_per_m", rearAntiRollBarNPerM));
    const float loadedFrontArbNmPerRad =
        config.getFloat("suspension.front_arb_nm_per_rad", -1.0F);
    const float loadedRearArbNmPerRad =
        config.getFloat("suspension.rear_arb_nm_per_rad", -1.0F);
    useDynamicRollStiffnessFraction =
        loadedFrontArbNmPerRad >= 0.0F && loadedRearArbNmPerRad >= 0.0F;
    frontAntiRollBarNmPerRad = useDynamicRollStiffnessFraction
        ? std::clamp(loadedFrontArbNmPerRad, 0.0F, 250000.0F)
        : 0.0F;
    rearAntiRollBarNmPerRad = useDynamicRollStiffnessFraction
        ? std::clamp(loadedRearArbNmPerRad, 0.0F, 250000.0F)
        : 0.0F;
    maxSuspensionCompressionM = std::clamp(
        config.getFloat("suspension.max_compression_m", maxSuspensionCompressionM),
        0.015F,
        0.30F);
    maxSuspensionDroopM =
        std::clamp(config.getFloat("suspension.max_droop_m", maxSuspensionDroopM), 0.005F, 0.20F);
    bumpStopRateNPerM = std::max(
        0.0F,
        config.getFloat("suspension.bump_stop_rate_n_per_m", bumpStopRateNPerM));
    bumpStopDampingNPerMps = std::max(
        0.0F,
        config.getFloat("suspension.bump_stop_damping_n_per_mps", bumpStopDampingNPerMps));
    frontStaticRideHeightM = std::clamp(
        config.getFloat("suspension.front_static_ride_height_m", frontStaticRideHeightM),
        0.015F,
        0.20F);
    rearStaticRideHeightM = std::clamp(
        config.getFloat("suspension.rear_static_ride_height_m", rearStaticRideHeightM),
        0.015F,
        0.22F);
    tireVerticalStiffnessNPerM = std::max(
        1000.0F,
        config.getFloat("suspension.tire_vertical_stiffness_n_per_m", tireVerticalStiffnessNPerM));
    tireVerticalDampingNPerMps = std::max(
        0.0F,
        config.getFloat("suspension.tire_vertical_damping_n_per_mps", tireVerticalDampingNPerMps));

    engineTorqueNm =
        std::max(0.0F, config.getFloat("powertrain.engine_torque_nm", engineTorqueNm));
    engineBrakingN =
        std::max(0.0F, config.getFloat("powertrain.engine_braking_n", engineBrakingN));
    drivetrainEfficiency = std::clamp(
        config.getFloat("powertrain.drivetrain_efficiency", drivetrainEfficiency),
        0.1F,
        1.0F);
    driveFrontFraction =
        std::clamp(config.getFloat("powertrain.drive_front_fraction", driveFrontFraction), 0.0F, 1.0F);
    differentialLoadBias = std::clamp(
        config.getFloat("powertrain.differential_load_bias", differentialLoadBias),
        0.0F,
        1.0F);
    wheelRadiusM =
        std::max(0.1F, config.getFloat("powertrain.wheel_radius_m", wheelRadiusM));
    wheelInertiaKgM2 =
        std::clamp(config.getFloat("powertrain.wheel_inertia_kg_m2", wheelInertiaKgM2), 0.05F, 8.0F);
    finalDriveRatio =
        std::max(0.1F, config.getFloat("powertrain.final_drive_ratio", finalDriveRatio));
    for (int gear = 0; gear < static_cast<int>(gearRatios.size()); ++gear) {
        const std::string key = "powertrain.gear_" + std::to_string(gear + 1);
        gearRatios[static_cast<std::size_t>(gear)] =
            std::max(0.01F, config.getFloat(key, gearRatios[static_cast<std::size_t>(gear)]));
    }
    idleRpm = std::max(300.0F, config.getFloat("powertrain.idle_rpm", idleRpm));
    redlineRpm = std::max(idleRpm + 1000.0F, config.getFloat("powertrain.redline_rpm", redlineRpm));
    shiftUpRpm = std::clamp(
        config.getFloat("powertrain.shift_up_rpm", shiftUpRpm),
        idleRpm + 500.0F,
        redlineRpm);
    shiftDownRpm = std::clamp(
        config.getFloat("powertrain.shift_down_rpm", shiftDownRpm),
        idleRpm,
        shiftUpRpm - 250.0F);
    const bool legacyAutomaticShift =
        config.getBool("powertrain.automatic_shift", automaticShift);
    automaticShift =
        config.getBool("powertrain.automatic_transmission", legacyAutomaticShift);

    brakeForceN = std::max(
        0.0F,
        config.getFloat("brakes.max_brake_force_n", config.getFloat("powertrain.brake_force_n", brakeForceN)));
    brakeBias = std::clamp(config.getFloat("brakes.brake_bias", brakeBias), 0.05F, 0.95F);
    brakeGamma = std::clamp(config.getFloat("brakes.brake_gamma", brakeGamma), 0.25F, 4.0F);

    aeroDragNPerMps2 = std::max(
        0.0F,
        config.getFloat("aero.drag_n_per_mps2", config.getFloat("resistance.aero_drag_n_per_mps2", aeroDragNPerMps2)));
    downforceNPerMps2 =
        std::max(0.0F, config.getFloat("aero.downforce_n_per_mps2", downforceNPerMps2));
    frontDownforceFraction =
        std::clamp(config.getFloat("aero.front_downforce_fraction", frontDownforceFraction), 0.05F, 0.95F);
    aeroReferenceFrontRideHeightM = std::clamp(
        config.getFloat("aero.reference_front_ride_height_m", aeroReferenceFrontRideHeightM),
        0.010F,
        0.20F);
    aeroReferenceRearRideHeightM = std::clamp(
        config.getFloat("aero.reference_rear_ride_height_m", aeroReferenceRearRideHeightM),
        0.010F,
        0.22F);
    groundEffectRideHeightScaleM = std::clamp(
        config.getFloat("aero.ground_effect_ride_height_scale_m", groundEffectRideHeightScaleM),
        0.006F,
        0.12F);
    maxGroundEffectMultiplier = std::clamp(
        config.getFloat("aero.max_ground_effect_multiplier", maxGroundEffectMultiplier),
        0.4F,
        5.0F);
    aeroStallRideHeightM = std::clamp(
        config.getFloat("aero.stall_ride_height_m", aeroStallRideHeightM),
        0.002F,
        0.10F);
    aeroStallDownforceMultiplier = std::clamp(
        config.getFloat("aero.stall_downforce_multiplier", aeroStallDownforceMultiplier),
        0.05F,
        1.0F);
    aeroCopShiftPerMeter =
        std::clamp(config.getFloat("aero.cop_shift_per_meter", aeroCopShiftPerMeter), 0.0F, 6.0F);
    aeroRideHeightBalanceSensitivity = std::clamp(
        config.getFloat("aero.ride_height_balance_sensitivity", aeroRideHeightBalanceSensitivity),
        0.0F,
        0.80F);
    aeroBrakeCopShift =
        std::clamp(config.getFloat("aero.brake_cop_shift", aeroBrakeCopShift), 0.0F, 0.12F);
    aeroStallCopShift =
        std::clamp(config.getFloat("aero.stall_cop_shift", aeroStallCopShift), 0.0F, 0.16F);
    aeroInstantLoadFraction = std::clamp(
        config.getFloat("aero.instant_load_fraction", aeroInstantLoadFraction),
        0.0F,
        0.50F);
    aeroYawDampingNmPerRadS = std::clamp(
        config.getFloat("aero.yaw_damping_nm_per_rad_s", aeroYawDampingNmPerRadS),
        0.0F,
        6000.0F);
    aeroYawDampingReferenceSpeedMps = std::clamp(
        config.getFloat("aero.yaw_damping_reference_speed_mps", aeroYawDampingReferenceSpeedMps),
        5.0F,
        120.0F);
    minFrontDownforceFraction = std::clamp(
        config.getFloat("aero.min_front_downforce_fraction", minFrontDownforceFraction),
        0.05F,
        0.95F);
    maxFrontDownforceFraction = std::clamp(
        config.getFloat("aero.max_front_downforce_fraction", maxFrontDownforceFraction),
        minFrontDownforceFraction,
        0.95F);
    const auto loadAeroPreset = [&](const char* prefix, AeroPresetConfig fallback) {
        AeroPresetConfig preset = fallback;
        const std::string base = std::string("aero_presets.") + prefix + ".";
        preset.downforceNPerMps2 =
            std::max(0.0F, config.getFloat(base + "downforce_n_per_mps2", preset.downforceNPerMps2));
        preset.dragNPerMps2 = std::max(
            0.0F,
            config.getFloat(
                base + "drag_coefficient",
                config.getFloat(base + "drag_n_per_mps2", preset.dragNPerMps2)));
        preset.frontDownforceFraction = std::clamp(
            config.getFloat(base + "front_downforce_fraction", preset.frontDownforceFraction),
            minFrontDownforceFraction,
            maxFrontDownforceFraction);
        preset.brakeCopShift =
            std::clamp(config.getFloat(base + "brake_cop_shift", preset.brakeCopShift), 0.0F, 0.12F);
        preset.stallRideHeightM =
            std::clamp(config.getFloat(base + "stall_height_m", preset.stallRideHeightM), 0.002F, 0.12F);
        preset.stallDownforceMultiplier = std::clamp(
            config.getFloat(base + "stall_reduction_factor", preset.stallDownforceMultiplier),
            0.05F,
            1.0F);
        return preset;
    };
    speedwayAeroPreset = loadAeroPreset(
        "speedway",
        AeroPresetConfig{
            downforceNPerMps2,
            aeroDragNPerMps2,
            frontDownforceFraction,
            aeroBrakeCopShift,
            aeroStallRideHeightM,
            aeroStallDownforceMultiplier,
        });
    roadCourseAeroPreset = loadAeroPreset("road_course", roadCourseAeroPreset);
    downforceNPerMps2 = speedwayAeroPreset.downforceNPerMps2;
    aeroDragNPerMps2 = speedwayAeroPreset.dragNPerMps2;
    frontDownforceFraction = speedwayAeroPreset.frontDownforceFraction;
    aeroBrakeCopShift = speedwayAeroPreset.brakeCopShift;
    aeroStallRideHeightM = speedwayAeroPreset.stallRideHeightM;
    aeroStallDownforceMultiplier = speedwayAeroPreset.stallDownforceMultiplier;
    rollingResistanceN =
        std::max(0.0F, config.getFloat("resistance.rolling_resistance_n", rollingResistanceN));
}

}  // namespace sim
