#include "physics/Vehicle.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace sim {
namespace {

constexpr float kGravity = 9.80665F;
constexpr float kLowSpeedThreshold = 0.15F;
constexpr float kTwoPi = 2.0F * std::numbers::pi_v<float>;
constexpr std::size_t kWheelCount = 4;
constexpr std::size_t kFrontLeft = 0;
constexpr std::size_t kFrontRight = 1;
constexpr std::size_t kRearLeft = 2;
constexpr std::size_t kRearRight = 3;
constexpr float kAutoShiftWheelspinSlipThreshold = 0.15F;
constexpr float kFirstGearAutoUpshiftMinSpeedMps = 15.0F;
constexpr float kSecondGearAutoUpshiftMinSpeedMps = 30.0F;
constexpr float kShiftCooldownSeconds = 0.10F;
constexpr float kAmbientTireTemperatureC = 31.0F;
constexpr float kInitialTireTemperatureC = 82.0F;

float dot(float ax, float az, float bx, float bz) {
    return ax * bx + az * bz;
}

float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

float square(float value) {
    return value * value;
}

float signOrZero(float value, float deadZone) {
    if (std::abs(value) <= deadZone) {
        return 0.0F;
    }
    return std::copysign(1.0F, value);
}

float smoothStep(float edge0, float edge1, float value) {
    const float t = std::clamp((value - edge0) / std::max(0.0001F, edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

float pacejkaLiteForce(
    float slip,
    float stiffness,
    float frictionLimit,
    float peakSlip,
    float shapeFactor,
    float curvatureFactor,
    float postPeakFalloff) {
    const float safeLimit = std::max(1.0F, frictionLimit);
    const float safePeakSlip = std::max(0.001F, peakSlip);
    const float safeShape = std::max(0.20F, shapeFactor);
    const float normalizedSlip = slip / safePeakSlip;
    const float stiffnessFactor =
        std::clamp(stiffness * safePeakSlip / (safeShape * safeLimit), 0.05F, 9.0F);
    const float shaped =
        stiffnessFactor * normalizedSlip -
        curvatureFactor * (stiffnessFactor * normalizedSlip - std::atan(stiffnessFactor * normalizedSlip));
    float force = safeLimit * std::sin(safeShape * std::atan(shaped));
    const float postPeak =
        smoothStep(1.0F, 2.35F, std::abs(normalizedSlip));
    force *= 1.0F - std::clamp(postPeakFalloff, 0.0F, 0.8F) * postPeak;
    return std::clamp(force, -safeLimit, safeLimit);
}

float tireLateralForce(
    float slipAngleRadians,
    float corneringStiffness,
    float frictionLimit,
    float peakSlipAngleRadians,
    float shapeFactor,
    float curvatureFactor,
    float postPeakFalloff) {
    return pacejkaLiteForce(
        -slipAngleRadians,
        corneringStiffness,
        frictionLimit,
        peakSlipAngleRadians,
        shapeFactor,
        curvatureFactor,
        postPeakFalloff);
}

float tireLongitudinalForce(
    float slipRatio,
    float longitudinalStiffness,
    float frictionLimit,
    float peakSlipRatio,
    float shapeFactor,
    float curvatureFactor,
    float postPeakFalloff) {
    return pacejkaLiteForce(
        slipRatio,
        longitudinalStiffness,
        frictionLimit,
        peakSlipRatio,
        shapeFactor,
        curvatureFactor,
        postPeakFalloff);
}

struct AxleForce {
    float longitudinalN = 0.0F;
    float lateralN = 0.0F;
    float usage = 0.0F;
    float longitudinalSlip = 0.0F;
};

struct WheelForce {
    float localLongitudinalN = 0.0F;
    float localLateralN = 0.0F;
    float bodyLongitudinalN = 0.0F;
    float bodyLateralN = 0.0F;
    float slipAngleRadians = 0.0F;
    float normalLoadN = 0.0F;
    float usage = 0.0F;
    float longitudinalSlip = 0.0F;
};

struct SplitForce {
    float leftN = 0.0F;
    float rightN = 0.0F;
};

struct CornerGeometry {
    float x = 0.0F;
    float z = 0.0F;
    bool front = false;
};

struct AeroState {
    float downforceN = 0.0F;
    float frontFraction = 0.45F;
    float stallIntensity = 0.0F;
};

AxleForce resolveCombinedSlip(
    float longitudinalDemandN,
    float lateralDemandN,
    float longitudinalLimitN,
    float lateralLimitN) {
    const float safeLongitudinalLimit = std::max(1.0F, longitudinalLimitN);
    const float safeLateralLimit = std::max(1.0F, lateralLimitN);
    const float longitudinalUsage = longitudinalDemandN / safeLongitudinalLimit;
    const float lateralUsage = lateralDemandN / safeLateralLimit;
    const float demandUsage = std::hypot(longitudinalUsage, lateralUsage);
    const float scale = demandUsage > 1.0F ? 1.0F / demandUsage : 1.0F;

    return {
        longitudinalDemandN * scale,
        lateralDemandN * scale,
        std::hypot(longitudinalUsage * scale, lateralUsage * scale),
        std::clamp(longitudinalUsage, -3.0F, 3.0F),
    };
}

float wheelFrictionLimit(
    float baseFriction,
    float normalLoadN,
    float referenceLoadN,
    float loadSensitivityCoeff,
    float minLoadEfficiency) {
    const float loadRatio = normalLoadN / std::max(1.0F, referenceLoadN);
    const float loadEfficiency = std::clamp(
        1.0F - loadSensitivityCoeff * (loadRatio - 1.0F),
        minLoadEfficiency,
        1.0F);
    return std::max(1.0F, baseFriction * loadEfficiency * normalLoadN);
}

float tireThermalGripScale(float temperatureC) {
    const float coldWarmup = smoothStep(45.0F, 88.0F, temperatureC);
    const float coldScale = 0.72F + 0.30F * coldWarmup;
    const float optimum =
        0.04F * (1.0F - std::abs(std::clamp((temperatureC - 96.0F) / 30.0F, -1.0F, 1.0F)));
    const float heatLoss = 1.0F - 0.34F * smoothStep(112.0F, 154.0F, temperatureC);
    return std::clamp(coldScale * heatLoss + optimum, 0.62F, 1.06F);
}

float updatedTireTemperature(
    float previousTemperatureC,
    const WheelForce& wheelForce,
    float tireLongitudinalSpeedMps,
    float peakSlipAngleRadians,
    float peakSlipRatio,
    float deltaSeconds) {
    const float speedMps = std::abs(tireLongitudinalSpeedMps);
    const float lateralSlipLoad =
        std::abs(wheelForce.slipAngleRadians) / std::max(0.01F, peakSlipAngleRadians);
    const float longitudinalSlipLoad =
        std::abs(wheelForce.longitudinalSlip) / std::max(0.02F, peakSlipRatio);
    const float nearLimitWork = square(std::max(0.0F, wheelForce.usage - 0.62F)) * 23.0F;
    const float slidingWork =
        std::max(0.0F, lateralSlipLoad - 0.75F) * 4.6F +
        std::max(0.0F, longitudinalSlipLoad - 0.75F) * 3.4F;
    const float rollingWork = wheelForce.usage * wheelForce.usage * (2.2F + std::min(speedMps, 85.0F) * 0.035F);
    const float heatingCPerSecond = rollingWork + nearLimitWork + slidingWork;
    const float coolingCPerSecond =
        (previousTemperatureC - kAmbientTireTemperatureC) * (0.055F + std::min(speedMps, 90.0F) * 0.0028F);
    return std::clamp(
        previousTemperatureC + (heatingCPerSecond - coolingCPerSecond) * deltaSeconds,
        kAmbientTireTemperatureC,
        176.0F);
}

SplitForce splitByWheelLoad(float totalValue, float leftLoadN, float rightLoadN, float loadBias) {
    const float totalLoad = std::max(1.0F, leftLoadN + rightLoadN);
    const float loadShare = leftLoadN / totalLoad;
    const float leftShare = std::clamp(0.5F + (loadShare - 0.5F) * loadBias, 0.15F, 0.85F);
    return {totalValue * leftShare, totalValue * (1.0F - leftShare)};
}

float relaxedSlipAngle(
    float previousSlipAngle,
    float tireLongitudinalSpeed,
    float tireLateralSpeed,
    float relaxationLengthM,
    float deltaSeconds) {
    const float safeSigma = std::max(0.03F, relaxationLengthM);
    const float absoluteLongitudinalSpeed = std::abs(tireLongitudinalSpeed);
    const float instantaneousSlip =
        std::atan2(tireLateralSpeed, absoluteLongitudinalSpeed + 0.55F);

    if (absoluteLongitudinalSpeed < 0.75F) {
        const float blend = 1.0F - std::exp(-deltaSeconds * 8.0F);
        return std::clamp(
            previousSlipAngle + (instantaneousSlip - previousSlipAngle) * blend,
            -0.70F,
            0.70F);
    }

    const float derivative =
        (tireLateralSpeed - absoluteLongitudinalSpeed * previousSlipAngle) / safeSigma;
    return std::clamp(previousSlipAngle + derivative * deltaSeconds, -0.70F, 0.70F);
}

WheelForce resolveWheelForce(
    float bodyLongitudinalSpeed,
    float bodyLateralSpeed,
    float steerAngle,
    float relaxedSlipAngleRadians,
    float wheelOmegaRadPerSec,
    float wheelRadiusM,
    float corneringStiffness,
    float longitudinalStiffness,
    float normalLoadN,
    float referenceLoadN,
    float lateralFriction,
    float longitudinalFriction,
    float lateralPeakSlipAngleRadians,
    float longitudinalPeakSlipRatio,
    float tireCurveShapeFactor,
    float tireCurveCurvatureFactor,
    float tirePostPeakFalloff,
    float loadSensitivityCoeff,
    float loadSensitivityMinEfficiency) {
    const float cosSteer = std::cos(steerAngle);
    const float sinSteer = std::sin(steerAngle);
    const float tireLongitudinalSpeed =
        bodyLongitudinalSpeed * cosSteer + bodyLateralSpeed * sinSteer;
    const float lateralFrictionLimit =
        wheelFrictionLimit(
            lateralFriction,
            normalLoadN,
            referenceLoadN,
            loadSensitivityCoeff,
            loadSensitivityMinEfficiency);
    const float longitudinalFrictionLimit =
        wheelFrictionLimit(
            longitudinalFriction,
            normalLoadN,
            referenceLoadN,
            loadSensitivityCoeff,
            loadSensitivityMinEfficiency);
    const float lateralDemand =
        tireLateralForce(
            relaxedSlipAngleRadians,
            corneringStiffness,
            lateralFrictionLimit,
            lateralPeakSlipAngleRadians,
            tireCurveShapeFactor,
            tireCurveCurvatureFactor,
            tirePostPeakFalloff);
    const float wheelSurfaceSpeed = wheelOmegaRadPerSec * wheelRadiusM;
    const float slipDenominator =
        std::max({std::abs(tireLongitudinalSpeed), std::abs(wheelSurfaceSpeed), 2.0F});
    const float slipRatio =
        std::clamp((wheelSurfaceSpeed - tireLongitudinalSpeed) / slipDenominator, -3.0F, 3.0F);
    const float longitudinalDemand =
        tireLongitudinalForce(
            slipRatio,
            longitudinalStiffness,
            longitudinalFrictionLimit,
            longitudinalPeakSlipRatio,
            tireCurveShapeFactor,
            tireCurveCurvatureFactor,
            tirePostPeakFalloff);
    const AxleForce localForce =
        resolveCombinedSlip(
            longitudinalDemand,
            lateralDemand,
            longitudinalFrictionLimit,
            lateralFrictionLimit);

    return {
        localForce.longitudinalN,
        localForce.lateralN,
        localForce.longitudinalN * cosSteer - localForce.lateralN * sinSteer,
        localForce.longitudinalN * sinSteer + localForce.lateralN * cosSteer,
        relaxedSlipAngleRadians,
        normalLoadN,
        localForce.usage,
        slipRatio,
    };
}

}  // namespace

Vehicle::Vehicle(VehicleConfig config) : config_(config) {
    reset();
}

void Vehicle::resetDynamicState() {
    currentGear_ = 1;
    previousShiftUp_ = false;
    previousShiftDown_ = false;
    shiftCooldownSeconds_ = 0.0F;
    rpmLimiterPhase_ = 0.0F;
    rpmLimiterCut_ = 0.0F;
    current_.rpm = config_.idleRpm;
    current_.gear = currentGear_;
    current_.brakeBias = config_.brakeBias;
    current_.frontRideHeightM = config_.frontStaticRideHeightM;
    current_.rearRideHeightM = config_.rearStaticRideHeightM;
    current_.aeroCenterOfPressure = config_.frontDownforceFraction;
    current_.frontLeftTireTemperatureC = kInitialTireTemperatureC;
    current_.frontRightTireTemperatureC = kInitialTireTemperatureC;
    current_.rearLeftTireTemperatureC = kInitialTireTemperatureC;
    current_.rearRightTireTemperatureC = kInitialTireTemperatureC;
    current_.frontLeftThermalGrip = tireThermalGripScale(kInitialTireTemperatureC);
    current_.frontRightThermalGrip = current_.frontLeftThermalGrip;
    current_.rearLeftThermalGrip = current_.frontLeftThermalGrip;
    current_.rearRightThermalGrip = current_.frontLeftThermalGrip;
}

void Vehicle::reset() {
    current_ = {};
    resetDynamicState();
    previous_ = current_;
}

void Vehicle::reset(float positionX, float positionZ, float yawRadians) {
    current_ = {};
    current_.positionX = positionX;
    current_.positionZ = positionZ;
    current_.yawRadians = yawRadians;
    resetDynamicState();
    previous_ = current_;
}

void Vehicle::step(
    const InputActions& input,
    float deltaSeconds,
    float gripMultiplier,
    float longitudinalGripMultiplier,
    float resistanceMultiplier,
    float bankRadians,
    float trackTangentX,
    float trackTangentZ) {
    previous_ = current_;

    const float safeDt = std::clamp(deltaSeconds, 0.0F, 0.05F);
    if (safeDt <= 0.0F) {
        return;
    }
    shiftCooldownSeconds_ = std::max(0.0F, shiftCooldownSeconds_ - safeDt);

    const float forwardX = std::sin(current_.yawRadians);
    const float forwardZ = std::cos(current_.yawRadians);
    const float rightX = forwardZ;
    const float rightZ = -forwardX;

    const float longitudinalSpeed =
        dot(current_.velocityX, current_.velocityZ, forwardX, forwardZ);
    const float lateralSpeed = dot(current_.velocityX, current_.velocityZ, rightX, rightZ);
    const float steeringSpeedFraction = std::clamp(
        std::abs(longitudinalSpeed) / std::max(1.0F, config_.steerSpeedThresholdMps),
        0.0F,
        1.0F);
    const float steeringScale =
        1.0F + (config_.highSpeedSteerScale - 1.0F) * steeringSpeedFraction;
    const float maxSteerLimit = config_.maxRoadWheelAngleRadians * steeringScale;
    const float steerAngle =
        std::clamp(
            std::clamp(input.steer, -1.0F, 1.0F) * config_.maxRoadWheelAngleRadians,
            -maxSteerLimit,
            maxSteerLimit);
    const float throttle = clamp01(input.throttle);
    const float brakeInput = std::pow(clamp01(input.brake), config_.brakeGamma);
    const float absoluteLongitudinalSpeed = std::abs(longitudinalSpeed);
    const float speedSquared = square(std::hypot(current_.velocityX, current_.velocityZ));

    updateGear(engineRpmForDrivenWheels(), input);
    const float engineRpm = engineRpmForDrivenWheels();
    const float gearRatio = gearRatioFor(currentGear_);
    const float limiterBlend =
        throttle > 0.05F
            ? smoothStep(config_.redlineRpm - 420.0F, config_.redlineRpm - 45.0F, engineRpm)
            : 0.0F;
    rpmLimiterPhase_ = std::fmod(
        rpmLimiterPhase_ + safeDt * (18.0F + limiterBlend * 18.0F),
        1.0F);
    const float limiterPulse = 0.5F + 0.5F * std::sin(kTwoPi * rpmLimiterPhase_);
    const float limiterTarget =
        limiterBlend * (0.28F + 0.72F * smoothStep(0.42F, 0.96F, limiterPulse));
    const float limiterResponse = 1.0F - std::exp(-safeDt * 30.0F);
    rpmLimiterCut_ += (limiterTarget - rpmLimiterCut_) * limiterResponse;
    if (limiterBlend <= 0.0F) {
        rpmLimiterCut_ *= std::exp(-safeDt * 12.0F);
    }

    const float rearDistance = config_.wheelbaseM * config_.frontWeightFraction;
    const float frontDistance = config_.wheelbaseM - rearDistance;
    const float halfTrack = config_.trackWidthM * 0.5F;
    const std::array<CornerGeometry, kWheelCount> corners{{
        {-halfTrack, frontDistance, true},
        {halfTrack, frontDistance, true},
        {-halfTrack, -rearDistance, false},
        {halfTrack, -rearDistance, false},
    }};

    std::array<float, kWheelCount> hubTravel{{
        current_.frontLeftWheelHubTravelM,
        current_.frontRightWheelHubTravelM,
        current_.rearLeftWheelHubTravelM,
        current_.rearRightWheelHubTravelM,
    }};
    std::array<float, kWheelCount> hubVelocity{{
        current_.frontLeftWheelHubVelocityMps,
        current_.frontRightWheelHubVelocityMps,
        current_.rearLeftWheelHubVelocityMps,
        current_.rearRightWheelHubVelocityMps,
    }};
    std::array<float, kWheelCount> wheelOmega{{
        current_.frontLeftWheelAngularVelocityRadPerSec,
        current_.frontRightWheelAngularVelocityRadPerSec,
        current_.rearLeftWheelAngularVelocityRadPerSec,
        current_.rearRightWheelAngularVelocityRadPerSec,
    }};
    std::array<float, kWheelCount> wheelRotation{{
        current_.frontLeftWheelRotationRadians,
        current_.frontRightWheelRotationRadians,
        current_.rearLeftWheelRotationRadians,
        current_.rearRightWheelRotationRadians,
    }};
    std::array<float, kWheelCount> relaxedSlip{{
        current_.frontLeftSlipAngleRadians,
        current_.frontRightSlipAngleRadians,
        current_.rearLeftSlipAngleRadians,
        current_.rearRightSlipAngleRadians,
    }};

    const float bankSin = std::sin(bankRadians);
    const float bankCos = std::cos(bankRadians);
    const float effectiveGravity = std::max(
        1.0F,
        kGravity * bankCos + (longitudinalSpeed * current_.yawRate) * bankSin);
    const float sprungMassKg =
        std::max(100.0F, config_.massKg - config_.unsprungMassPerWheelKg * 4.0F);
    const float unsprungWeightN = config_.unsprungMassPerWheelKg * effectiveGravity;
    const float staticFrontSprungLoadN =
        sprungMassKg * effectiveGravity * config_.frontWeightFraction;
    const float staticRearSprungLoadN =
        sprungMassKg * effectiveGravity - staticFrontSprungLoadN;

    const auto computeAero = [&](float frontRideHeightM, float rearRideHeightM) {
        const float frontRide = std::max(0.001F, frontRideHeightM);
        const float rearRide = std::max(0.001F, rearRideHeightM);
        const float frontEffect = std::exp(std::clamp(
            (config_.aeroReferenceFrontRideHeightM - frontRide) /
                std::max(0.001F, config_.groundEffectRideHeightScaleM),
            -0.55F,
            1.45F));
        const float rearEffect = std::exp(std::clamp(
            (config_.aeroReferenceRearRideHeightM - rearRide) /
                std::max(0.001F, config_.groundEffectRideHeightScaleM),
            -0.55F,
            1.45F));
        const float groundEffect =
            std::clamp((frontEffect * 0.48F + rearEffect * 0.52F), 0.45F, config_.maxGroundEffectMultiplier);
        const float minimumRide = std::min(frontRide, rearRide);
        const float stallBlend =
            smoothStep(config_.aeroStallRideHeightM, config_.aeroStallRideHeightM + 0.018F, minimumRide);
        const float stallMultiplier =
            config_.aeroStallDownforceMultiplier +
            (1.0F - config_.aeroStallDownforceMultiplier) * stallBlend;
        const float rake = rearRide - frontRide;
        const float tunnelFrontShare =
            frontEffect / std::max(0.001F, frontEffect + rearEffect);
        const float rideHeightBalanceShift =
            (tunnelFrontShare - 0.48F) * config_.aeroRideHeightBalanceSensitivity;
        const float brakingShift =
            config_.aeroBrakeCopShift *
            std::clamp(brakeInput + std::max(0.0F, -current_.longitudinalG) * 0.35F, 0.0F, 1.0F);
        const float frontStall =
            1.0F - smoothStep(config_.aeroStallRideHeightM, config_.aeroStallRideHeightM + 0.014F, frontRide);
        const float rearStall =
            1.0F - smoothStep(config_.aeroStallRideHeightM, config_.aeroStallRideHeightM + 0.014F, rearRide);
        const float stallBalanceShift =
            (rearStall - frontStall) * config_.aeroStallCopShift;
        const float frontFraction = std::clamp(
            config_.frontDownforceFraction +
                rake * config_.aeroCopShiftPerMeter +
                rideHeightBalanceShift +
                brakingShift +
                stallBalanceShift,
            config_.minFrontDownforceFraction,
            config_.maxFrontDownforceFraction);
        return AeroState{
            speedSquared * config_.downforceNPerMps2 * groundEffect * stallMultiplier,
            frontFraction,
            1.0F - stallBlend,
        };
    };
    AeroState aero = computeAero(
        current_.frontRideHeightM > 0.0F ? current_.frontRideHeightM : config_.frontStaticRideHeightM,
        current_.rearRideHeightM > 0.0F ? current_.rearRideHeightM : config_.rearStaticRideHeightM);
    const float suspensionFrontAeroN = aero.downforceN * aero.frontFraction;
    const float suspensionRearAeroN = aero.downforceN - suspensionFrontAeroN;

    std::array<float, kWheelCount> suspensionCompression{};
    std::array<float, kWheelCount> suspensionForce{};
    std::array<float, kWheelCount> tireNormalLoad{};
    std::array<float, kWheelCount> mountTravel{};
    std::array<float, kWheelCount> mountVelocity{};
    float floorStrike = aero.stallIntensity;

    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        const bool front = corners[wheel].front;
        const float springRate = front ? config_.frontSpringRateNPerM : config_.rearSpringRateNPerM;
        const float damper = front ? config_.frontDamperNPerMps : config_.rearDamperNPerMps;
        const float staticCornerLoad =
            (front ? staticFrontSprungLoadN : staticRearSprungLoadN) * 0.5F;
        const float staticCompression = staticCornerLoad / std::max(1.0F, springRate);
        mountTravel[wheel] =
            current_.chassisHeaveM -
            current_.chassisPitchRadians * corners[wheel].z +
            current_.chassisRollRadians * corners[wheel].x;
        mountVelocity[wheel] =
            current_.chassisHeaveVelocityMps -
            current_.chassisPitchRate * corners[wheel].z +
            current_.chassisRollRate * corners[wheel].x;

        suspensionCompression[wheel] =
            std::max(0.0F, staticCompression + hubTravel[wheel] - mountTravel[wheel]);
        const float compressionRate = hubVelocity[wheel] - mountVelocity[wheel];
        float springForce = springRate * suspensionCompression[wheel] + damper * compressionRate;
        const float bumpStart = staticCompression + config_.maxSuspensionCompressionM;
        if (suspensionCompression[wheel] > bumpStart) {
            const float bumpTravel = suspensionCompression[wheel] - bumpStart;
            springForce += config_.bumpStopRateNPerM * bumpTravel +
                           config_.bumpStopDampingNPerMps * std::max(0.0F, compressionRate);
            floorStrike = std::max(floorStrike, std::clamp(bumpTravel * 32.0F, 0.0F, 1.0F));
        }
        suspensionForce[wheel] = std::max(0.0F, springForce);
    }

    const auto applyAntiRollBar = [&](std::size_t leftWheel, std::size_t rightWheel, float rateNPerM) {
        const float twist = suspensionCompression[leftWheel] - suspensionCompression[rightWheel];
        const float force = rateNPerM * twist * 0.5F;
        suspensionForce[leftWheel] = std::max(0.0F, suspensionForce[leftWheel] + force);
        suspensionForce[rightWheel] = std::max(0.0F, suspensionForce[rightWheel] - force);
    };
    applyAntiRollBar(kFrontLeft, kFrontRight, config_.frontAntiRollBarNPerM);
    applyAntiRollBar(kRearLeft, kRearRight, config_.rearAntiRollBarNPerM);

    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        const bool front = corners[wheel].front;
        const float staticCornerLoad =
            (front ? staticFrontSprungLoadN : staticRearSprungLoadN) * 0.5F;
        const float tireStaticCompression =
            (staticCornerLoad + unsprungWeightN) / std::max(1.0F, config_.tireVerticalStiffnessNPerM);
        const float tireCompression = std::max(0.0F, tireStaticCompression - hubTravel[wheel]);
        const float tireForce =
            config_.tireVerticalStiffnessNPerM * tireCompression -
            config_.tireVerticalDampingNPerMps * hubVelocity[wheel];
        tireNormalLoad[wheel] = std::max(0.0F, tireForce);

        const float hubAcceleration =
            (tireNormalLoad[wheel] - suspensionForce[wheel] - unsprungWeightN) /
            std::max(1.0F, config_.unsprungMassPerWheelKg);
        hubVelocity[wheel] += hubAcceleration * safeDt;
        hubTravel[wheel] += hubVelocity[wheel] * safeDt;
        const float lowerTravelLimit = -(config_.maxSuspensionCompressionM + 0.055F);
        const float upperTravelLimit = config_.maxSuspensionDroopM + 0.045F;
        if (hubTravel[wheel] < lowerTravelLimit) {
            hubTravel[wheel] = lowerTravelLimit;
            hubVelocity[wheel] = std::max(0.0F, hubVelocity[wheel]) * 0.25F;
        }
        if (hubTravel[wheel] > upperTravelLimit) {
            hubTravel[wheel] = upperTravelLimit;
            hubVelocity[wheel] = std::min(0.0F, hubVelocity[wheel]) * 0.25F;
        }
    }

    const float longitudinalLoadTransfer =
        config_.massKg * current_.longitudinalG * kGravity * config_.centerOfMassHeightM /
        std::max(0.1F, config_.wheelbaseM);
    const float totalLateralTransfer =
        config_.massKg * current_.lateralG * kGravity * config_.centerOfMassHeightM /
        std::max(0.1F, config_.trackWidthM);
    const float frontLateralTransfer = totalLateralTransfer * config_.frontRollStiffnessFraction;
    const float rearLateralTransfer = totalLateralTransfer - frontLateralTransfer;
    tireNormalLoad[kFrontLeft] += -longitudinalLoadTransfer * 0.5F + frontLateralTransfer * 0.5F;
    tireNormalLoad[kFrontRight] += -longitudinalLoadTransfer * 0.5F - frontLateralTransfer * 0.5F;
    tireNormalLoad[kRearLeft] += longitudinalLoadTransfer * 0.5F + rearLateralTransfer * 0.5F;
    tireNormalLoad[kRearRight] += longitudinalLoadTransfer * 0.5F - rearLateralTransfer * 0.5F;
    for (float& normalLoad : tireNormalLoad) {
        normalLoad = std::max(0.0F, normalLoad);
    }

    const float sumSuspensionForce =
        suspensionForce[kFrontLeft] + suspensionForce[kFrontRight] +
        suspensionForce[kRearLeft] + suspensionForce[kRearRight];
    const float heaveAcceleration =
        (sumSuspensionForce - sprungMassKg * effectiveGravity - aero.downforceN) /
        std::max(1.0F, sprungMassKg);
    float pitchMoment = frontDistance * suspensionFrontAeroN - rearDistance * suspensionRearAeroN -
                        sprungMassKg * current_.longitudinalG * kGravity * config_.centerOfMassHeightM;
    float rollMoment =
        sprungMassKg * current_.lateralG * kGravity * config_.centerOfMassHeightM;
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        pitchMoment -= corners[wheel].z * suspensionForce[wheel];
        rollMoment += corners[wheel].x * suspensionForce[wheel];
    }

    current_.chassisHeaveVelocityMps += heaveAcceleration * safeDt;
    current_.chassisHeaveVelocityMps *= std::exp(-0.35F * safeDt);
    current_.chassisHeaveM += current_.chassisHeaveVelocityMps * safeDt;
    current_.chassisPitchRate += pitchMoment / std::max(1.0F, config_.pitchInertiaKgM2) * safeDt;
    current_.chassisPitchRate *= std::exp(-1.15F * safeDt);
    current_.chassisPitchRadians += current_.chassisPitchRate * safeDt;
    current_.chassisRollRate += rollMoment / std::max(1.0F, config_.rollInertiaKgM2) * safeDt;
    current_.chassisRollRate *= std::exp(-1.35F * safeDt);
    current_.chassisRollRadians += current_.chassisRollRate * safeDt;

    const auto clampState = [](float& value, float& velocity, float minimum, float maximum) {
        if (value < minimum) {
            value = minimum;
            velocity = std::max(0.0F, velocity) * 0.35F;
        } else if (value > maximum) {
            value = maximum;
            velocity = std::min(0.0F, velocity) * 0.35F;
        }
    };
    clampState(current_.chassisHeaveM, current_.chassisHeaveVelocityMps, -0.090F, 0.080F);
    clampState(current_.chassisPitchRadians, current_.chassisPitchRate, -0.115F, 0.115F);
    clampState(current_.chassisRollRadians, current_.chassisRollRate, -0.115F, 0.115F);

    std::array<float, kWheelCount> visualSuspensionTravel{};
    std::array<float, kWheelCount> rideHeight{};
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        const float updatedMountTravel =
            current_.chassisHeaveM -
            current_.chassisPitchRadians * corners[wheel].z +
            current_.chassisRollRadians * corners[wheel].x;
        visualSuspensionTravel[wheel] = hubTravel[wheel] - updatedMountTravel;
        rideHeight[wheel] =
            (corners[wheel].front ? config_.frontStaticRideHeightM : config_.rearStaticRideHeightM) +
            updatedMountTravel;
    }
    current_.frontRideHeightM =
        std::max(0.0F, (rideHeight[kFrontLeft] + rideHeight[kFrontRight]) * 0.5F);
    current_.rearRideHeightM =
        std::max(0.0F, (rideHeight[kRearLeft] + rideHeight[kRearRight]) * 0.5F);
    const float minimumRideHeight =
        std::min({rideHeight[kFrontLeft], rideHeight[kFrontRight], rideHeight[kRearLeft], rideHeight[kRearRight]});
    if (minimumRideHeight < config_.aeroStallRideHeightM) {
        floorStrike = std::max(
            floorStrike,
            std::clamp((config_.aeroStallRideHeightM - minimumRideHeight) / 0.020F, 0.0F, 1.0F));
    }
    aero = computeAero(current_.frontRideHeightM, current_.rearRideHeightM);
    floorStrike = std::max(floorStrike, aero.stallIntensity);
    const float instantAeroLoad = aero.downforceN * config_.aeroInstantLoadFraction;
    const float instantFrontAeroLoad = instantAeroLoad * aero.frontFraction;
    const float instantRearAeroLoad = instantAeroLoad - instantFrontAeroLoad;
    tireNormalLoad[kFrontLeft] += instantFrontAeroLoad * 0.5F;
    tireNormalLoad[kFrontRight] += instantFrontAeroLoad * 0.5F;
    tireNormalLoad[kRearLeft] += instantRearAeroLoad * 0.5F;
    tireNormalLoad[kRearRight] += instantRearAeroLoad * 0.5F;
    for (float& normalLoad : tireNormalLoad) {
        normalLoad = std::max(0.0F, normalLoad);
    }

    const float leftNormalLoad = tireNormalLoad[kFrontLeft] + tireNormalLoad[kRearLeft];
    const float rightNormalLoad = tireNormalLoad[kFrontRight] + tireNormalLoad[kRearRight];
    const float totalNormalLoad = std::max(1.0F, leftNormalLoad + rightNormalLoad);
    const float lateralTransferRatio = std::abs(leftNormalLoad - rightNormalLoad) / totalNormalLoad;
    const float lateralGripScale = 1.0F - std::clamp(
        lateralTransferRatio * config_.lateralLoadTransferGripLoss,
        0.0F,
        0.18F);
    if (longitudinalGripMultiplier < 0.0F) {
        longitudinalGripMultiplier = gripMultiplier;
    }
    const float lateralEffectiveFriction =
        config_.frictionCoefficient * std::max(0.05F, gripMultiplier) * lateralGripScale;
    const float longitudinalEffectiveFriction =
        config_.frictionCoefficient * std::max(0.05F, longitudinalGripMultiplier) * lateralGripScale;
    std::array<float, kWheelCount> tireTemperature{{
        std::max(kAmbientTireTemperatureC, current_.frontLeftTireTemperatureC),
        std::max(kAmbientTireTemperatureC, current_.frontRightTireTemperatureC),
        std::max(kAmbientTireTemperatureC, current_.rearLeftTireTemperatureC),
        std::max(kAmbientTireTemperatureC, current_.rearRightTireTemperatureC),
    }};
    std::array<float, kWheelCount> thermalGrip{};
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        thermalGrip[wheel] = tireThermalGripScale(tireTemperature[wheel]);
    }

    const float driveTorqueTotal =
        throttle * engineTorqueAt(engineRpm, throttle) *
        std::clamp(1.0F - rpmLimiterCut_ * 0.72F, 0.0F, 1.0F) *
        gearRatio * config_.finalDriveRatio * config_.drivetrainEfficiency;
    const float engineBrakeTorqueTotal =
        (1.0F - throttle) * config_.engineBrakingN * config_.wheelRadiusM;
    std::array<float, kWheelCount> driveTorque{};
    std::array<float, kWheelCount> brakeTorque{};
    std::array<float, kWheelCount> engineBrakeTorque{};

    const SplitForce frontDriveSplit = splitByWheelLoad(
        driveTorqueTotal * config_.driveFrontFraction,
        tireNormalLoad[kFrontLeft],
        tireNormalLoad[kFrontRight],
        config_.differentialLoadBias);
    const SplitForce rearDriveSplit = splitByWheelLoad(
        driveTorqueTotal * (1.0F - config_.driveFrontFraction),
        tireNormalLoad[kRearLeft],
        tireNormalLoad[kRearRight],
        config_.differentialLoadBias);
    driveTorque[kFrontLeft] = frontDriveSplit.leftN;
    driveTorque[kFrontRight] = frontDriveSplit.rightN;
    driveTorque[kRearLeft] = rearDriveSplit.leftN;
    driveTorque[kRearRight] = rearDriveSplit.rightN;

    const SplitForce frontEngineBrakeSplit = splitByWheelLoad(
        engineBrakeTorqueTotal * config_.driveFrontFraction,
        tireNormalLoad[kFrontLeft],
        tireNormalLoad[kFrontRight],
        config_.differentialLoadBias);
    const SplitForce rearEngineBrakeSplit = splitByWheelLoad(
        engineBrakeTorqueTotal * (1.0F - config_.driveFrontFraction),
        tireNormalLoad[kRearLeft],
        tireNormalLoad[kRearRight],
        config_.differentialLoadBias);
    const std::array<float, kWheelCount> engineBrakeMagnitude{{
        frontEngineBrakeSplit.leftN,
        frontEngineBrakeSplit.rightN,
        rearEngineBrakeSplit.leftN,
        rearEngineBrakeSplit.rightN,
    }};

    const float brakeTorqueTotal = brakeInput * config_.brakeForceN * config_.wheelRadiusM;
    brakeTorque[kFrontLeft] = brakeTorqueTotal * config_.brakeBias * 0.5F;
    brakeTorque[kFrontRight] = brakeTorqueTotal * config_.brakeBias * 0.5F;
    brakeTorque[kRearLeft] = brakeTorqueTotal * (1.0F - config_.brakeBias) * 0.5F;
    brakeTorque[kRearRight] = brakeTorqueTotal * (1.0F - config_.brakeBias) * 0.5F;

    std::array<float, kWheelCount> bodyLongitudinalSpeed{};
    std::array<float, kWheelCount> bodyLateralSpeed{};
    std::array<float, kWheelCount> tireLongitudinalSpeed{};
    std::array<float, kWheelCount> tireLateralSpeed{};
    std::array<float, kWheelCount> wheelOffsetWorldX{};
    std::array<float, kWheelCount> wheelOffsetWorldZ{};
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        wheelOffsetWorldX[wheel] = rightX * corners[wheel].x + forwardX * corners[wheel].z;
        wheelOffsetWorldZ[wheel] = rightZ * corners[wheel].x + forwardZ * corners[wheel].z;
        const float contactVelocityX =
            current_.velocityX + current_.yawRate * (rightX * corners[wheel].z - forwardX * corners[wheel].x);
        const float contactVelocityZ =
            current_.velocityZ + current_.yawRate * (rightZ * corners[wheel].z - forwardZ * corners[wheel].x);
        bodyLongitudinalSpeed[wheel] =
            dot(contactVelocityX, contactVelocityZ, forwardX, forwardZ);
        bodyLateralSpeed[wheel] =
            dot(contactVelocityX, contactVelocityZ, rightX, rightZ);
        const float steer = corners[wheel].front ? steerAngle : 0.0F;
        const float cosSteer = std::cos(steer);
        const float sinSteer = std::sin(steer);
        tireLongitudinalSpeed[wheel] =
            bodyLongitudinalSpeed[wheel] * cosSteer + bodyLateralSpeed[wheel] * sinSteer;
        tireLateralSpeed[wheel] =
            bodyLateralSpeed[wheel] * cosSteer - bodyLongitudinalSpeed[wheel] * sinSteer;
        relaxedSlip[wheel] = relaxedSlipAngle(
            relaxedSlip[wheel],
            tireLongitudinalSpeed[wheel],
            tireLateralSpeed[wheel],
            config_.tireRelaxationLengthM,
            safeDt);

        float opposeSign = signOrZero(wheelOmega[wheel], 0.08F);
        if (opposeSign == 0.0F) {
            opposeSign = signOrZero(tireLongitudinalSpeed[wheel], 0.25F);
        }
        if (opposeSign == 0.0F) {
            opposeSign = signOrZero(driveTorque[wheel], 0.5F);
        }
        brakeTorque[wheel] = -opposeSign * brakeTorque[wheel];
        engineBrakeTorque[wheel] = -opposeSign * engineBrakeMagnitude[wheel];
    }

    const float frontWheelStiffness = config_.frontCorneringStiffness * 0.5F;
    const float rearWheelStiffness = config_.rearCorneringStiffness * 0.5F;
    const float tireLoadReferenceN = std::max(1.0F, config_.tireLoadReferenceNormalN);

    const WheelForce frontLeft = resolveWheelForce(
        bodyLongitudinalSpeed[kFrontLeft],
        bodyLateralSpeed[kFrontLeft],
        steerAngle,
        relaxedSlip[kFrontLeft],
        wheelOmega[kFrontLeft],
        config_.wheelRadiusM,
        frontWheelStiffness,
        config_.tireLongitudinalStiffness,
        tireNormalLoad[kFrontLeft],
        tireLoadReferenceN,
        lateralEffectiveFriction * thermalGrip[kFrontLeft],
        longitudinalEffectiveFriction * thermalGrip[kFrontLeft],
        config_.lateralPeakSlipAngleRadians,
        config_.longitudinalPeakSlipRatio,
        config_.tireCurveShapeFactor,
        config_.tireCurveCurvatureFactor,
        config_.tirePostPeakFalloff,
        config_.tireLoadSensitivityCoeff,
        config_.tireLoadSensitivityMinEfficiency);
    const WheelForce frontRight = resolveWheelForce(
        bodyLongitudinalSpeed[kFrontRight],
        bodyLateralSpeed[kFrontRight],
        steerAngle,
        relaxedSlip[kFrontRight],
        wheelOmega[kFrontRight],
        config_.wheelRadiusM,
        frontWheelStiffness,
        config_.tireLongitudinalStiffness,
        tireNormalLoad[kFrontRight],
        tireLoadReferenceN,
        lateralEffectiveFriction * thermalGrip[kFrontRight],
        longitudinalEffectiveFriction * thermalGrip[kFrontRight],
        config_.lateralPeakSlipAngleRadians,
        config_.longitudinalPeakSlipRatio,
        config_.tireCurveShapeFactor,
        config_.tireCurveCurvatureFactor,
        config_.tirePostPeakFalloff,
        config_.tireLoadSensitivityCoeff,
        config_.tireLoadSensitivityMinEfficiency);
    const WheelForce rearLeft = resolveWheelForce(
        bodyLongitudinalSpeed[kRearLeft],
        bodyLateralSpeed[kRearLeft],
        0.0F,
        relaxedSlip[kRearLeft],
        wheelOmega[kRearLeft],
        config_.wheelRadiusM,
        rearWheelStiffness,
        config_.tireLongitudinalStiffness,
        tireNormalLoad[kRearLeft],
        tireLoadReferenceN,
        lateralEffectiveFriction * thermalGrip[kRearLeft],
        longitudinalEffectiveFriction * thermalGrip[kRearLeft],
        config_.lateralPeakSlipAngleRadians,
        config_.longitudinalPeakSlipRatio,
        config_.tireCurveShapeFactor,
        config_.tireCurveCurvatureFactor,
        config_.tirePostPeakFalloff,
        config_.tireLoadSensitivityCoeff,
        config_.tireLoadSensitivityMinEfficiency);
    const WheelForce rearRight = resolveWheelForce(
        bodyLongitudinalSpeed[kRearRight],
        bodyLateralSpeed[kRearRight],
        0.0F,
        relaxedSlip[kRearRight],
        wheelOmega[kRearRight],
        config_.wheelRadiusM,
        rearWheelStiffness,
        config_.tireLongitudinalStiffness,
        tireNormalLoad[kRearRight],
        tireLoadReferenceN,
        lateralEffectiveFriction * thermalGrip[kRearRight],
        longitudinalEffectiveFriction * thermalGrip[kRearRight],
        config_.lateralPeakSlipAngleRadians,
        config_.longitudinalPeakSlipRatio,
        config_.tireCurveShapeFactor,
        config_.tireCurveCurvatureFactor,
        config_.tirePostPeakFalloff,
        config_.tireLoadSensitivityCoeff,
        config_.tireLoadSensitivityMinEfficiency);
    const std::array<WheelForce, kWheelCount> wheelForces{{
        frontLeft,
        frontRight,
        rearLeft,
        rearRight,
    }};
    std::array<float, kWheelCount> wheelForceWorldX{};
    std::array<float, kWheelCount> wheelForceWorldZ{};
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        wheelForceWorldX[wheel] =
            forwardX * wheelForces[wheel].bodyLongitudinalN +
            rightX * wheelForces[wheel].bodyLateralN;
        wheelForceWorldZ[wheel] =
            forwardZ * wheelForces[wheel].bodyLongitudinalN +
            rightZ * wheelForces[wheel].bodyLateralN;
    }
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        tireTemperature[wheel] = updatedTireTemperature(
            tireTemperature[wheel],
            wheelForces[wheel],
            tireLongitudinalSpeed[wheel],
            config_.lateralPeakSlipAngleRadians,
            config_.longitudinalPeakSlipRatio,
            safeDt);
        thermalGrip[wheel] = tireThermalGripScale(tireTemperature[wheel]);
    }

    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        const float wheelTorque =
            driveTorque[wheel] + engineBrakeTorque[wheel] + brakeTorque[wheel] -
            wheelForces[wheel].localLongitudinalN * config_.wheelRadiusM;
        wheelOmega[wheel] += wheelTorque / std::max(0.05F, config_.wheelInertiaKgM2) * safeDt;
        if (brakeInput > 0.4F && std::abs(tireLongitudinalSpeed[wheel]) < 0.35F &&
            std::abs(wheelOmega[wheel]) < 1.2F && std::abs(driveTorque[wheel]) < 4.0F) {
            wheelOmega[wheel] = 0.0F;
        }
        wheelOmega[wheel] = std::clamp(wheelOmega[wheel], -520.0F, 520.0F);
        wheelRotation[wheel] += wheelOmega[wheel] * safeDt;
    }

    const float dragForce = -longitudinalSpeed * absoluteLongitudinalSpeed * config_.aeroDragNPerMps2;
    const float motionSign = signOrZero(longitudinalSpeed, kLowSpeedThreshold);
    const float rollingForce =
        -motionSign * (absoluteLongitudinalSpeed > kLowSpeedThreshold
                           ? config_.rollingResistanceN * std::max(0.1F, resistanceMultiplier)
                           : 0.0F);

    float totalForceWorldX = 0.0F;
    float totalForceWorldZ = 0.0F;
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        totalForceWorldX += wheelForceWorldX[wheel];
        totalForceWorldZ += wheelForceWorldZ[wheel];
    }
    const float longitudinalExternalForce = dragForce + rollingForce;
    totalForceWorldX += forwardX * longitudinalExternalForce;
    totalForceWorldZ += forwardZ * longitudinalExternalForce;
    // Banking gravity lateral force: decompose relative to the track tangent.
    // When the car's heading aligns with the track direction, banking is
    // captured by the effective-gravity normal load and produces no net
    // body-lateral push. The lateral component only appears when the
    // car's heading deviates from the track tangent (e.g. sliding sideways).
    const float trackTangentLength = std::hypot(trackTangentX, trackTangentZ);
    float bankLateralScale = 1.0F;
    if (trackTangentLength > 0.01F) {
        // Dot product of car's forward vector with track tangent gives
        // cos(heading error). The lateral gravity push scales with
        // sin(heading error) = sqrt(1 - cos^2).
        const float cosHeadingError = std::clamp(
            (forwardX * trackTangentX + forwardZ * trackTangentZ) / trackTangentLength,
            -1.0F, 1.0F);
        bankLateralScale = std::sqrt(std::max(0.0F, 1.0F - cosHeadingError * cosHeadingError));
    }
    const float gravityLateralForce = -config_.massKg * kGravity * bankSin * bankLateralScale;
    totalForceWorldX += rightX * gravityLateralForce;
    totalForceWorldZ += rightZ * gravityLateralForce;
    const float longitudinalForce = dot(totalForceWorldX, totalForceWorldZ, forwardX, forwardZ);
    const float lateralForce = dot(totalForceWorldX, totalForceWorldZ, rightX, rightZ);
    const float longitudinalAcceleration = longitudinalForce / config_.massKg;
    const float lateralAcceleration = lateralForce / config_.massKg;

    const float accelerationX = totalForceWorldX / config_.massKg;
    const float accelerationZ = totalForceWorldZ / config_.massKg;
    current_.velocityX += accelerationX * safeDt;
    current_.velocityZ += accelerationZ * safeDt;

    if (input.brake > 0.0F && std::abs(longitudinalSpeed) < kLowSpeedThreshold &&
        std::hypot(current_.velocityX, current_.velocityZ) < 0.5F) {
        current_.velocityX = 0.0F;
        current_.velocityZ = 0.0F;
    }

    float yawMoment = 0.0F;
    for (std::size_t wheel = 0; wheel < kWheelCount; ++wheel) {
        yawMoment +=
            wheelOffsetWorldZ[wheel] * wheelForceWorldX[wheel] -
            wheelOffsetWorldX[wheel] * wheelForceWorldZ[wheel];
    }
    current_.yawRate += yawMoment / std::max(1.0F, config_.yawInertiaKgM2) * safeDt;
    current_.yawRadians += current_.yawRate * safeDt;
    current_.positionX += current_.velocityX * safeDt;
    current_.positionZ += current_.velocityZ * safeDt;

    current_.frontLeftWheelHubTravelM = hubTravel[kFrontLeft];
    current_.frontRightWheelHubTravelM = hubTravel[kFrontRight];
    current_.rearLeftWheelHubTravelM = hubTravel[kRearLeft];
    current_.rearRightWheelHubTravelM = hubTravel[kRearRight];
    current_.frontLeftWheelHubVelocityMps = hubVelocity[kFrontLeft];
    current_.frontRightWheelHubVelocityMps = hubVelocity[kFrontRight];
    current_.rearLeftWheelHubVelocityMps = hubVelocity[kRearLeft];
    current_.rearRightWheelHubVelocityMps = hubVelocity[kRearRight];
    current_.frontLeftSuspensionTravelM = visualSuspensionTravel[kFrontLeft];
    current_.frontRightSuspensionTravelM = visualSuspensionTravel[kFrontRight];
    current_.rearLeftSuspensionTravelM = visualSuspensionTravel[kRearLeft];
    current_.rearRightSuspensionTravelM = visualSuspensionTravel[kRearRight];
    current_.frontLeftWheelAngularVelocityRadPerSec = wheelOmega[kFrontLeft];
    current_.frontRightWheelAngularVelocityRadPerSec = wheelOmega[kFrontRight];
    current_.rearLeftWheelAngularVelocityRadPerSec = wheelOmega[kRearLeft];
    current_.rearRightWheelAngularVelocityRadPerSec = wheelOmega[kRearRight];
    current_.frontLeftWheelRotationRadians = wheelRotation[kFrontLeft];
    current_.frontRightWheelRotationRadians = wheelRotation[kFrontRight];
    current_.rearLeftWheelRotationRadians = wheelRotation[kRearLeft];
    current_.rearRightWheelRotationRadians = wheelRotation[kRearRight];
    current_.wheelRotationRadians =
        (wheelRotation[kFrontLeft] + wheelRotation[kFrontRight] +
         wheelRotation[kRearLeft] + wheelRotation[kRearRight]) *
        0.25F;

    current_.longitudinalSpeedMps = longitudinalSpeed;
    current_.lateralSpeedMps = lateralSpeed;
    current_.steeringAngleRadians = steerAngle;
    current_.frontSlipAngleRadians =
        (frontLeft.slipAngleRadians + frontRight.slipAngleRadians) * 0.5F;
    current_.rearSlipAngleRadians =
        (rearLeft.slipAngleRadians + rearRight.slipAngleRadians) * 0.5F;
    current_.frontLeftSlipAngleRadians = frontLeft.slipAngleRadians;
    current_.frontRightSlipAngleRadians = frontRight.slipAngleRadians;
    current_.rearLeftSlipAngleRadians = rearLeft.slipAngleRadians;
    current_.rearRightSlipAngleRadians = rearRight.slipAngleRadians;
    current_.frontLongitudinalSlip =
        (frontLeft.longitudinalSlip + frontRight.longitudinalSlip) * 0.5F;
    current_.rearLongitudinalSlip =
        (rearLeft.longitudinalSlip + rearRight.longitudinalSlip) * 0.5F;
    current_.frontLeftLongitudinalSlip = frontLeft.longitudinalSlip;
    current_.frontRightLongitudinalSlip = frontRight.longitudinalSlip;
    current_.rearLeftLongitudinalSlip = rearLeft.longitudinalSlip;
    current_.rearRightLongitudinalSlip = rearRight.longitudinalSlip;
    current_.frontNormalLoadN = frontLeft.normalLoadN + frontRight.normalLoadN;
    current_.rearNormalLoadN = rearLeft.normalLoadN + rearRight.normalLoadN;
    current_.frontLeftNormalLoadN = frontLeft.normalLoadN;
    current_.frontRightNormalLoadN = frontRight.normalLoadN;
    current_.rearLeftNormalLoadN = rearLeft.normalLoadN;
    current_.rearRightNormalLoadN = rearRight.normalLoadN;
    current_.frontLateralForceN = frontLeft.bodyLateralN + frontRight.bodyLateralN;
    current_.rearLateralForceN = rearLeft.bodyLateralN + rearRight.bodyLateralN;
    current_.frontLeftLateralForceN = frontLeft.bodyLateralN;
    current_.frontRightLateralForceN = frontRight.bodyLateralN;
    current_.rearLeftLateralForceN = rearLeft.bodyLateralN;
    current_.rearRightLateralForceN = rearRight.bodyLateralN;
    current_.frontLongitudinalForceN =
        frontLeft.bodyLongitudinalN + frontRight.bodyLongitudinalN;
    current_.rearLongitudinalForceN =
        rearLeft.bodyLongitudinalN + rearRight.bodyLongitudinalN;
    current_.frontTireUsage = std::max(frontLeft.usage, frontRight.usage);
    current_.rearTireUsage = std::max(rearLeft.usage, rearRight.usage);
    current_.frontLeftTireUsage = frontLeft.usage;
    current_.frontRightTireUsage = frontRight.usage;
    current_.rearLeftTireUsage = rearLeft.usage;
    current_.rearRightTireUsage = rearRight.usage;
    current_.frontLeftTireTemperatureC = tireTemperature[kFrontLeft];
    current_.frontRightTireTemperatureC = tireTemperature[kFrontRight];
    current_.rearLeftTireTemperatureC = tireTemperature[kRearLeft];
    current_.rearRightTireTemperatureC = tireTemperature[kRearRight];
    current_.frontLeftThermalGrip = thermalGrip[kFrontLeft];
    current_.frontRightThermalGrip = thermalGrip[kFrontRight];
    current_.rearLeftThermalGrip = thermalGrip[kRearLeft];
    current_.rearRightThermalGrip = thermalGrip[kRearRight];
    current_.engineForceN = driveTorqueTotal / std::max(0.05F, config_.wheelRadiusM);
    current_.brakeForceN = brakeInput * config_.brakeForceN;
    current_.dragForceN = dragForce;
    current_.downforceN = aero.downforceN;
    current_.aeroCenterOfPressure = aero.frontFraction;
    current_.floorStrikeIntensity = std::clamp(floorStrike, 0.0F, 1.0F);
    current_.brakeBias = config_.brakeBias;
    current_.lateralG = lateralAcceleration / kGravity;
    current_.longitudinalG = longitudinalAcceleration / kGravity;
    const float limiterBounceRpm =
        rpmLimiterCut_ * (80.0F + 190.0F * limiterPulse);
    current_.rpm = std::clamp(
        engineRpmForDrivenWheels() - limiterBounceRpm,
        config_.idleRpm,
        config_.redlineRpm);
    current_.gear = currentGear_;
    refreshSpeedTelemetry();
}

void Vehicle::resolveBoundary(
    float outwardNormalX,
    float outwardNormalZ,
    float penetrationM,
    float restitution,
    float contactOffsetX,
    float contactOffsetZ) {
    const float correctionX = outwardNormalX * penetrationM;
    const float correctionZ = outwardNormalZ * penetrationM;
    current_.positionX -= correctionX;
    current_.positionZ -= correctionZ;
    previous_.positionX -= correctionX;
    previous_.positionZ -= correctionZ;

    const float contactVelocityX = current_.velocityX + current_.yawRate * contactOffsetZ;
    const float contactVelocityZ = current_.velocityZ - current_.yawRate * contactOffsetX;
    const float outwardVelocity =
        contactVelocityX * outwardNormalX + contactVelocityZ * outwardNormalZ;
    if (outwardVelocity > 0.0F) {
        const float inverseMass = 1.0F / std::max(1.0F, config_.massKg);
        const float inverseYawInertia = 1.0F / std::max(1.0F, config_.yawInertiaKgM2);
        const float contactCrossNormal =
            contactOffsetZ * outwardNormalX - contactOffsetX * outwardNormalZ;
        const float impulse =
            (1.0F + std::clamp(restitution, 0.0F, 1.0F)) * outwardVelocity /
            std::max(0.0001F, inverseMass + contactCrossNormal * contactCrossNormal * inverseYawInertia);
        current_.velocityX -= impulse * inverseMass * outwardNormalX;
        current_.velocityZ -= impulse * inverseMass * outwardNormalZ;
        current_.yawRate -= impulse * contactCrossNormal * inverseYawInertia;
        current_.yawRate *= std::abs(contactCrossNormal) < 0.0001F ? 0.45F : 0.82F;
        current_.chassisRollRate *= 0.65F;
        current_.chassisPitchRate *= 0.75F;
    }
    refreshSpeedTelemetry();
}

float Vehicle::collisionHalfLengthM() const {
    return std::max(config_.wheelbaseM * 0.5F + 1.02F, config_.wheelbaseM * 0.82F);
}

float Vehicle::collisionHalfWidthM() const {
    return std::max(config_.trackWidthM * 0.5F - 0.05F, 0.90F);
}

void Vehicle::applyVelocityDamping(float dampingPerSecond, float deltaSeconds) {
    const float damping = std::max(0.0F, dampingPerSecond);
    const float safeDt = std::max(0.0F, deltaSeconds);
    const float factor = std::exp(-damping * safeDt);
    current_.velocityX *= factor;
    current_.velocityZ *= factor;
    current_.yawRate *= std::exp(-damping * 0.35F * safeDt);
    current_.frontLeftWheelAngularVelocityRadPerSec *= factor;
    current_.frontRightWheelAngularVelocityRadPerSec *= factor;
    current_.rearLeftWheelAngularVelocityRadPerSec *= factor;
    current_.rearRightWheelAngularVelocityRadPerSec *= factor;
    refreshSpeedTelemetry();
}

void Vehicle::setBrakeBias(float brakeBias) {
    config_.brakeBias = std::clamp(brakeBias, 0.05F, 0.95F);
    current_.brakeBias = config_.brakeBias;
    previous_.brakeBias = config_.brakeBias;
}

void Vehicle::setAutomaticShift(bool automaticShift) {
    config_.automaticShift = automaticShift;
}

VehicleState Vehicle::interpolated(float alpha) const {
    const auto blend = [alpha](float from, float to) { return from + (to - from) * alpha; };
    VehicleState result = current_;
    result.positionX = blend(previous_.positionX, current_.positionX);
    result.positionZ = blend(previous_.positionZ, current_.positionZ);
    result.yawRadians = blend(previous_.yawRadians, current_.yawRadians);
    result.yawRate = blend(previous_.yawRate, current_.yawRate);
    result.chassisHeaveM = blend(previous_.chassisHeaveM, current_.chassisHeaveM);
    result.chassisPitchRadians = blend(previous_.chassisPitchRadians, current_.chassisPitchRadians);
    result.chassisRollRadians = blend(previous_.chassisRollRadians, current_.chassisRollRadians);
    result.frontRideHeightM = blend(previous_.frontRideHeightM, current_.frontRideHeightM);
    result.rearRideHeightM = blend(previous_.rearRideHeightM, current_.rearRideHeightM);
    result.floorStrikeIntensity =
        blend(previous_.floorStrikeIntensity, current_.floorStrikeIntensity);
    result.wheelRotationRadians =
        blend(previous_.wheelRotationRadians, current_.wheelRotationRadians);
    result.frontLeftWheelRotationRadians =
        blend(previous_.frontLeftWheelRotationRadians, current_.frontLeftWheelRotationRadians);
    result.frontRightWheelRotationRadians =
        blend(previous_.frontRightWheelRotationRadians, current_.frontRightWheelRotationRadians);
    result.rearLeftWheelRotationRadians =
        blend(previous_.rearLeftWheelRotationRadians, current_.rearLeftWheelRotationRadians);
    result.rearRightWheelRotationRadians =
        blend(previous_.rearRightWheelRotationRadians, current_.rearRightWheelRotationRadians);
    result.frontLeftWheelHubTravelM =
        blend(previous_.frontLeftWheelHubTravelM, current_.frontLeftWheelHubTravelM);
    result.frontRightWheelHubTravelM =
        blend(previous_.frontRightWheelHubTravelM, current_.frontRightWheelHubTravelM);
    result.rearLeftWheelHubTravelM =
        blend(previous_.rearLeftWheelHubTravelM, current_.rearLeftWheelHubTravelM);
    result.rearRightWheelHubTravelM =
        blend(previous_.rearRightWheelHubTravelM, current_.rearRightWheelHubTravelM);
    result.frontLeftSuspensionTravelM =
        blend(previous_.frontLeftSuspensionTravelM, current_.frontLeftSuspensionTravelM);
    result.frontRightSuspensionTravelM =
        blend(previous_.frontRightSuspensionTravelM, current_.frontRightSuspensionTravelM);
    result.rearLeftSuspensionTravelM =
        blend(previous_.rearLeftSuspensionTravelM, current_.rearLeftSuspensionTravelM);
    result.rearRightSuspensionTravelM =
        blend(previous_.rearRightSuspensionTravelM, current_.rearRightSuspensionTravelM);
    return result;
}

float Vehicle::gearRatioFor(int gear) const {
    const int index = std::clamp(gear, 1, static_cast<int>(config_.gearRatios.size())) - 1;
    return config_.gearRatios[static_cast<std::size_t>(index)];
}

float Vehicle::engineRpmForSpeed(float absoluteLongitudinalSpeed) const {
    const float wheelRadiansPerSecond = absoluteLongitudinalSpeed / config_.wheelRadiusM;
    const float engineRadiansPerSecond =
        wheelRadiansPerSecond * gearRatioFor(currentGear_) * config_.finalDriveRatio;
    const float coupledRpm = engineRadiansPerSecond * 60.0F / kTwoPi;
    return std::clamp(std::max(config_.idleRpm, coupledRpm), config_.idleRpm, config_.redlineRpm);
}

float Vehicle::engineRpmForDrivenWheels() const {
    const float frontWeight = config_.driveFrontFraction;
    const float rearWeight = 1.0F - config_.driveFrontFraction;
    const float frontOmega =
        (std::abs(current_.frontLeftWheelAngularVelocityRadPerSec) +
         std::abs(current_.frontRightWheelAngularVelocityRadPerSec)) *
        0.5F;
    const float rearOmega =
        (std::abs(current_.rearLeftWheelAngularVelocityRadPerSec) +
         std::abs(current_.rearRightWheelAngularVelocityRadPerSec)) *
        0.5F;
    const float averageDrivenOmega =
        frontOmega * frontWeight + rearOmega * rearWeight;
    const float coupledRpm =
        averageDrivenOmega * gearRatioFor(currentGear_) * config_.finalDriveRatio * 60.0F / kTwoPi;
    return std::clamp(std::max(config_.idleRpm, coupledRpm), config_.idleRpm, config_.redlineRpm);
}

float Vehicle::engineTorqueAt(float rpm, float throttle) const {
    if (throttle <= 0.0F) {
        return 0.0F;
    }
    const float normalizedRpm = std::clamp(
        (rpm - config_.idleRpm) / (config_.redlineRpm - config_.idleRpm),
        0.0F,
        1.0F);
    const float midRangeBulge = std::sin(normalizedRpm * std::numbers::pi_v<float>);
    const float redlineCut = std::clamp(
        (config_.redlineRpm - rpm) / std::max(1.0F, config_.redlineRpm * 0.04F),
        0.0F,
        1.0F);
    return config_.engineTorqueNm * (0.82F + 0.24F * midRangeBulge) * redlineCut;
}

void Vehicle::updateGear(float engineRpm, const InputActions& input) {
    const bool shiftUpEdge = input.shiftUp && !previousShiftUp_;
    const bool shiftDownEdge = input.shiftDown && !previousShiftDown_;

    const auto commitShift = [&](int nextGear) {
        const int clampedGear =
            std::clamp(nextGear, 1, static_cast<int>(config_.gearRatios.size()));
        if (clampedGear != currentGear_) {
            currentGear_ = clampedGear;
            shiftCooldownSeconds_ = kShiftCooldownSeconds;
            return true;
        }
        return false;
    };

    bool shifted = false;
    const bool canShift = shiftCooldownSeconds_ <= 0.0F;
    if (canShift && shiftUpEdge) {
        shifted = commitShift(currentGear_ + 1);
    }
    if (!shifted && canShift && shiftDownEdge) {
        shifted = commitShift(currentGear_ - 1);
    }

    if (!shifted && canShift && config_.automaticShift) {
        const float minimumUpshiftSpeed =
            currentGear_ == 1 ? kFirstGearAutoUpshiftMinSpeedMps :
            currentGear_ == 2 ? kSecondGearAutoUpshiftMinSpeedMps :
            0.0F;
        const float rearPowerSlip =
            std::max(current_.rearLeftLongitudinalSlip, current_.rearRightLongitudinalSlip);
        const bool launchWheelspin =
            currentGear_ <= 2 &&
            input.throttle > 0.25F &&
            rearPowerSlip > kAutoShiftWheelspinSlipThreshold;
        const bool allowAutomaticUpshift =
            current_.speedMps >= minimumUpshiftSpeed && !launchWheelspin;
        if (engineRpm >= config_.shiftUpRpm &&
            allowAutomaticUpshift &&
            currentGear_ < static_cast<int>(config_.gearRatios.size())) {
            shifted = commitShift(currentGear_ + 1);
        }
        if (!shifted && engineRpm <= config_.shiftDownRpm && currentGear_ > 1) {
            commitShift(currentGear_ - 1);
        }
    }

    previousShiftUp_ = input.shiftUp;
    previousShiftDown_ = input.shiftDown;
}

void Vehicle::refreshSpeedTelemetry() {
    current_.speedMps = std::hypot(current_.velocityX, current_.velocityZ);
    const float forwardX = std::sin(current_.yawRadians);
    const float forwardZ = std::cos(current_.yawRadians);
    const float rightX = forwardZ;
    const float rightZ = -forwardX;
    current_.longitudinalSpeedMps =
        dot(current_.velocityX, current_.velocityZ, forwardX, forwardZ);
    current_.lateralSpeedMps = dot(current_.velocityX, current_.velocityZ, rightX, rightZ);
    current_.gear = currentGear_;
    current_.rpm = engineRpmForDrivenWheels();
}

void Vehicle::setAeroPreset(int preset) {
    aeroPreset_ = std::clamp(preset, 0, 1);
    if (aeroPreset_ == 0) {
        // Speedway: lower drag, lower downforce, more pitch/yaw sensitivity
        config_.aeroDragNPerMps2 = 0.72F;
        config_.downforceNPerMps2 = 1.95F;
        config_.frontDownforceFraction = 0.43F;
        config_.maxGroundEffectMultiplier = 2.90F;
        config_.groundEffectRideHeightScaleM = 0.028F;
    } else {
        // Road course: higher downforce and drag
        config_.aeroDragNPerMps2 = 1.10F;
        config_.downforceNPerMps2 = 3.20F;
        config_.frontDownforceFraction = 0.46F;
        config_.maxGroundEffectMultiplier = 2.50F;
        config_.groundEffectRideHeightScaleM = 0.034F;
    }
}

const char* Vehicle::aeroPresetName() const {
    return aeroPreset_ == 0 ? "SPEEDWAY" : "ROAD COURSE";
}

}  // namespace sim
