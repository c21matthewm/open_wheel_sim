#include "ffb/ForceFeedbackManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>

#include "config/ConfigFile.h"

namespace sim {
namespace {

constexpr float kSteeringTorqueNormalizerNm = 720.0F;
constexpr float kSteeringAngleNormalizer = 0.35F;
constexpr float kSpeedScaleMps = 80.0F;

float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

float smoothStep(float edge0, float edge1, float value) {
    const float t = std::clamp((value - edge0) / std::max(0.0001F, edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

}  // namespace

void ForceFeedbackConfig::load(const ConfigFile& config) {
    enabled = config.getBool("enabled", enabled);
    masterStrength =
        std::clamp(config.getFloat("master_strength", masterStrength), 0.0F, 2.0F);
    centeringStrength =
        std::clamp(config.getFloat("centering_strength", centeringStrength), 0.0F, 2.0F);
    damping = std::clamp(config.getFloat("damping", damping), 0.0F, 2.0F);
    aligningTorqueScale =
        std::clamp(config.getFloat("aligning_torque_scale", aligningTorqueScale), 0.0F, 2.0F);
    pneumaticTrailM =
        std::clamp(config.getFloat("pneumatic_trail_m", pneumaticTrailM), 0.0F, 0.20F);
    mechanicalTrailM =
        std::clamp(config.getFloat("mechanical_trail_m", mechanicalTrailM), 0.0F, 0.12F);
    const float peakSlipAngleDegrees = std::clamp(
        config.getFloat("peak_slip_angle_deg", peakSlipAngleRadians * 180.0F / std::numbers::pi_v<float>),
        2.0F,
        18.0F);
    peakSlipAngleRadians = peakSlipAngleDegrees * std::numbers::pi_v<float> / 180.0F;
    understeerLightening =
        std::clamp(config.getFloat("understeer_lightening", understeerLightening), 0.0F, 5.0F);
    grassVibration =
        std::clamp(config.getFloat("grass_vibration", grassVibration), 0.0F, 2.0F);
    collisionJolt =
        std::clamp(config.getFloat("collision_jolt", collisionJolt), 0.0F, 2.0F);
    maxForce = std::clamp(config.getFloat("max_force", maxForce), 0.05F, 1.0F);
}

ForceFeedbackManager::~ForceFeedbackManager() {
    shutdown();
}

void ForceFeedbackManager::initialize(const ForceFeedbackConfig& config) {
    config_ = config;
    initialized_ = true;
    if (!config_.enabled) {
        status_ = "FFB: disabled in config";
        return;
    }
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) {
        status_ = std::string("FFB: SDL haptic init failed: ") + SDL_GetError();
        initialized_ = false;
        return;
    }
    status_ = "FFB: waiting for haptic wheel";
}

void ForceFeedbackManager::shutdown() {
    closeDevice();
    if (initialized_) {
        SDL_QuitSubSystem(SDL_INIT_HAPTIC);
        initialized_ = false;
    }
}

void ForceFeedbackManager::update(
    const VehicleState& vehicle,
    const RaceTelemetry& race,
    const InputActions& /*input*/,
    const SDLJoystickInput::Device* activeDevice,
    float deltaSeconds) {
    if (!initialized_ || !config_.enabled) {
        smoothedForce_ = 0.0F;
        return;
    }

    if (activeDevice == nullptr || activeDevice->handle == nullptr) {
        closeDevice();
        status_ = "FFB: no selected wheel/haptic device";
        return;
    }
    if (haptic_ == nullptr || joystickId_ != activeDevice->id) {
        tryOpenDevice(activeDevice);
    }

    const float force = computeForce(vehicle, race, deltaSeconds);
    if (haptic_ != nullptr && backend_ != Backend::None) {
        streamForce(force);
    }
}

void ForceFeedbackManager::closeDevice() {
    if (haptic_ != nullptr && effectId_ >= 0) {
        SDL_StopHapticEffect(haptic_, effectId_);
        SDL_DestroyHapticEffect(haptic_, effectId_);
        effectId_ = -1;
    }
    if (haptic_ != nullptr) {
        SDL_StopHapticEffects(haptic_);
        SDL_CloseHaptic(haptic_);
        haptic_ = nullptr;
    }
    joystickId_ = 0;
    backend_ = Backend::None;
    smoothedForce_ = 0.0F;
}

void ForceFeedbackManager::tryOpenDevice(const SDLJoystickInput::Device* device) {
    closeDevice();
    if (device == nullptr || device->handle == nullptr || !device->isHaptic) {
        status_ = "FFB: selected device is not SDL-haptic";
        return;
    }

    haptic_ = SDL_OpenHapticFromJoystick(device->handle);
    if (haptic_ == nullptr) {
        status_ = std::string("FFB: open from joystick failed: ") + SDL_GetError();
        return;
    }
    joystickId_ = device->id;

    const Uint32 features = SDL_GetHapticFeatures(haptic_);
    if ((features & SDL_HAPTIC_GAIN) != 0) {
        SDL_SetHapticGain(haptic_, 100);
    }
    if ((features & SDL_HAPTIC_AUTOCENTER) != 0) {
        SDL_SetHapticAutocenter(haptic_, 0);
    }

    SDL_HapticEffect effect = makeConstantEffect(0.0F);
    if ((features & SDL_HAPTIC_CONSTANT) != 0 &&
        SDL_HapticEffectSupported(haptic_, &effect)) {
        effectId_ = SDL_CreateHapticEffect(haptic_, &effect);
        if (effectId_ >= 0 && SDL_RunHapticEffect(haptic_, effectId_, SDL_HAPTIC_INFINITY)) {
            backend_ = Backend::Constant;
            status_ = std::string("FFB: SDL constant force active on ") + device->name.data();
            return;
        }
    }

    effect = makeSpringEffect(0.0F);
    if ((features & SDL_HAPTIC_SPRING) != 0 &&
        SDL_HapticEffectSupported(haptic_, &effect)) {
        effectId_ = SDL_CreateHapticEffect(haptic_, &effect);
        if (effectId_ >= 0 && SDL_RunHapticEffect(haptic_, effectId_, SDL_HAPTIC_INFINITY)) {
            backend_ = Backend::Spring;
            status_ = std::string("FFB: SDL spring fallback active on ") + device->name.data();
            return;
        }
    }

    effect = makeLeftRightEffect(0.0F);
    if ((features & SDL_HAPTIC_LEFTRIGHT) != 0 &&
        SDL_HapticEffectSupported(haptic_, &effect)) {
        effectId_ = SDL_CreateHapticEffect(haptic_, &effect);
        if (effectId_ >= 0) {
            backend_ = Backend::LeftRight;
            status_ = std::string("FFB: SDL left/right rumble fallback on ") + device->name.data();
            return;
        }
    }

    if (SDL_HapticRumbleSupported(haptic_) && SDL_InitHapticRumble(haptic_)) {
        backend_ = Backend::Rumble;
        status_ = std::string("FFB: simple rumble fallback on ") + device->name.data();
        return;
    }

    status_ = std::string("FFB: no supported force/rumble effects on ") + device->name.data();
    closeDevice();
}

SDL_HapticEffect ForceFeedbackManager::makeConstantEffect(float force) const {
    SDL_HapticEffect effect{};
    effect.type = SDL_HAPTIC_CONSTANT;
    effect.constant.type = SDL_HAPTIC_CONSTANT;
    effect.constant.direction.type = SDL_HAPTIC_STEERING_AXIS;
    effect.constant.direction.dir[0] = 0;
    effect.constant.length = SDL_HAPTIC_INFINITY;
    effect.constant.level = forceToSint16(force);
    return effect;
}

SDL_HapticEffect ForceFeedbackManager::makeSpringEffect(float force) const {
    const Sint16 coefficient = forceToSint16(std::clamp(std::abs(force), 0.18F, 1.0F));
    SDL_HapticEffect effect{};
    effect.type = SDL_HAPTIC_SPRING;
    effect.condition.type = SDL_HAPTIC_SPRING;
    effect.condition.direction.type = SDL_HAPTIC_STEERING_AXIS;
    effect.condition.length = SDL_HAPTIC_INFINITY;
    effect.condition.right_sat[0] = 0x7FFFU;
    effect.condition.left_sat[0] = 0x7FFFU;
    effect.condition.right_coeff[0] = -coefficient;
    effect.condition.left_coeff[0] = coefficient;
    effect.condition.deadband[0] = 0;
    effect.condition.center[0] = 0;
    return effect;
}

SDL_HapticEffect ForceFeedbackManager::makeLeftRightEffect(float force) const {
    const Uint16 magnitude =
        static_cast<Uint16>(std::clamp(std::abs(force), 0.0F, 1.0F) * 65535.0F);
    SDL_HapticEffect effect{};
    effect.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.type = SDL_HAPTIC_LEFTRIGHT;
    effect.leftright.length = 80;
    effect.leftright.large_magnitude = magnitude;
    effect.leftright.small_magnitude = static_cast<Uint16>(magnitude * 0.55F);
    return effect;
}

float ForceFeedbackManager::computeForce(
    const VehicleState& vehicle,
    const RaceTelemetry& race,
    float deltaSeconds) {
    const float safeDt = std::max(0.0001F, deltaSeconds);
    elapsedSeconds_ += safeDt;

    const float speedFraction = clamp01(vehicle.speedMps / kSpeedScaleMps);
    const float steeringVelocity =
        (vehicle.steeringAngleRadians - previousSteeringAngle_) / safeDt;
    previousSteeringAngle_ = vehicle.steeringAngleRadians;

    const float peakSlip = std::max(0.01F, config_.peakSlipAngleRadians);
    const auto trailForSlip = [&](float slipAngleRadians) {
        const float slip = std::abs(slipAngleRadians);
        const float pneumaticFalloff =
            (1.0F - smoothStep(peakSlip * 0.42F, peakSlip * 1.05F, slip) * 0.82F) *
            (1.0F - smoothStep(peakSlip * 1.00F, peakSlip * 1.55F, slip) * 0.78F);
        return config_.mechanicalTrailM + config_.pneumaticTrailM * std::clamp(pneumaticFalloff, 0.0F, 1.0F);
    };
    const bool hasSolverTrailTelemetry =
        std::abs(vehicle.frontLeftAligningTrailM) + std::abs(vehicle.frontRightAligningTrailM) > 0.0001F;
    const float leftTrail =
        hasSolverTrailTelemetry ? vehicle.frontLeftAligningTrailM : trailForSlip(vehicle.frontLeftSlipAngleRadians);
    const float rightTrail =
        hasSolverTrailTelemetry ? vehicle.frontRightAligningTrailM : trailForSlip(vehicle.frontRightSlipAngleRadians);
    const float aligningTorqueNm =
        -(vehicle.frontLeftLateralForceN * leftTrail + vehicle.frontRightLateralForceN * rightTrail);
    const float maximumFrontSlip =
        std::max(std::abs(vehicle.frontLeftSlipAngleRadians), std::abs(vehicle.frontRightSlipAngleRadians));
    const float understeerFactor = std::clamp(
        1.0F -
            smoothStep(peakSlip * 0.96F, peakSlip * 1.08F, maximumFrontSlip) *
                config_.understeerLightening,
        0.08F,
        1.0F);
    const float satForce =
        (aligningTorqueNm / kSteeringTorqueNormalizerNm) *
        config_.aligningTorqueScale *
        understeerFactor;

    const float normalizedSteer =
        vehicle.steeringAngleRadians / kSteeringAngleNormalizer;
    const float centeringForce =
        -normalizedSteer * config_.centeringStrength * (1.0F - speedFraction);
    const float dampingForce =
        -(steeringVelocity / 8.0F) * config_.damping;

    float vibration = 0.0F;
    if (race.surface == TrackSurface::Grass) {
        vibration = std::sin(elapsedSeconds_ * 55.0F) * config_.grassVibration;
    } else if (race.surface == TrackSurface::Apron) {
        vibration = std::sin(elapsedSeconds_ * 42.0F) * config_.grassVibration * 0.22F;
    }

    if (race.wallContact && !previousWallContact_) {
        collisionEnvelope_ = std::max(collisionEnvelope_, config_.collisionJolt);
        collisionSign_ = race.lateralOffsetM >= 0.0F ? -1.0F : 1.0F;
    }
    previousWallContact_ = race.wallContact;
    const float collisionForce = collisionSign_ * collisionEnvelope_;
    collisionEnvelope_ *= std::exp(-safeDt * 13.0F);

    const float rawForce =
        (satForce + centeringForce + dampingForce + vibration + collisionForce) *
        config_.masterStrength;
    const float clipped =
        std::clamp(rawForce, -config_.maxForce, config_.maxForce);
    const float smoothing = std::clamp(safeDt * 18.0F, 0.0F, 1.0F);
    smoothedForce_ += (clipped - smoothedForce_) * smoothing;
    return std::clamp(smoothedForce_, -config_.maxForce, config_.maxForce);
}

void ForceFeedbackManager::streamForce(float force) {
    if (haptic_ == nullptr) {
        return;
    }

    if (backend_ == Backend::Constant) {
        SDL_HapticEffect effect = makeConstantEffect(force);
        SDL_UpdateHapticEffect(haptic_, effectId_, &effect);
        return;
    }

    if (backend_ == Backend::Spring) {
        SDL_HapticEffect effect = makeSpringEffect(force);
        SDL_UpdateHapticEffect(haptic_, effectId_, &effect);
        return;
    }

    if (backend_ == Backend::LeftRight) {
        SDL_HapticEffect effect = makeLeftRightEffect(force);
        if (SDL_UpdateHapticEffect(haptic_, effectId_, &effect)) {
            SDL_RunHapticEffect(haptic_, effectId_, 1);
        }
        return;
    }

    if (backend_ == Backend::Rumble) {
        SDL_PlayHapticRumble(haptic_, std::clamp(std::abs(force), 0.0F, 1.0F), 80);
    }
}

Sint16 ForceFeedbackManager::forceToSint16(float force) const {
    const float clipped = std::clamp(force, -1.0F, 1.0F);
    return static_cast<Sint16>(clipped * 32767.0F);
}

}  // namespace sim
