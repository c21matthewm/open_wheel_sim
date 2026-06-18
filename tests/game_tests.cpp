#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

#include "config/ConfigFile.h"
#include "game/LapTimer.h"
#include "game/RaceSession.h"
#include "game/Track.h"
#include "physics/Vehicle.h"
#include "physics/VehicleConfig.h"

namespace {

bool near(float lhs, float rhs, float epsilon = 0.01F) {
    return std::abs(lhs - rhs) <= epsilon;
}

constexpr float kPhysicsDt = 1.0F / 360.0F;

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Expected config directory argument\n";
        return 1;
    }

    sim::ConfigFile configFile;
    if (!configFile.load(std::string(argv[1]) + "/track_oval_default.json")) {
        std::cerr << configFile.error() << '\n';
        return 1;
    }

    sim::TrackConfig config;
    config.load(configFile);
    sim::Track track(config);

    const float expectedLength =
        2.0F * config.straightLengthM +
        2.0F * config.shortChuteLengthM +
        2.0F * std::numbers::pi_v<float> * config.cornerRadiusM;
    if (!require(near(track.lapLengthM(), expectedLength), "Oval lap length is incorrect")) {
        return 1;
    }

    const sim::TrackPose start = track.poseAtDistance(0.0F);
    if (!require(
            near(start.centerX, config.shortChuteLengthM * 0.5F + config.cornerRadiusM) &&
                near(start.centerZ, 0.0F) &&
                near(start.yawRadians, 0.0F),
            "Start pose is incorrect")) {
        return 1;
    }

    const float fullBankRadians = config.bankingDegrees * std::numbers::pi_v<float> / 180.0F;
    const float turn4Entry = track.halfStraightLengthM();
    const float turn4Exit = turn4Entry + std::numbers::pi_v<float> * 0.5F * config.cornerRadiusM;
    const float runout = config.bankTransitionRunoutM;
    if (!require(
            near(track.poseAtDistance(turn4Entry - runout).bankRadians, 0.0F, 0.002F) &&
                near(track.poseAtDistance(turn4Entry).bankRadians, fullBankRadians * 0.5F, 0.004F) &&
                near(track.poseAtDistance(turn4Entry + runout).bankRadians, fullBankRadians, 0.004F) &&
                near(track.poseAtDistance(turn4Exit - runout).bankRadians, fullBankRadians, 0.004F) &&
                near(track.poseAtDistance(turn4Exit).bankRadians, fullBankRadians * 0.5F, 0.004F) &&
                near(track.poseAtDistance(turn4Exit + runout).bankRadians, 0.0F, 0.002F),
            "Banking easement should be centered around turn entry/exit")) {
        return 1;
    }

    for (int sampleIndex = 0; sampleIndex < 200; ++sampleIndex) {
        const float distance =
            track.lapLengthM() * static_cast<float>(sampleIndex) / 200.0F;
        const sim::TrackPose pose = track.poseAtDistance(distance);
        const sim::TrackSample centerline = track.sample(pose.centerX, pose.centerZ);
        const float rawError = std::abs(centerline.distanceM - distance);
        const float circularError = std::min(rawError, track.lapLengthM() - rawError);
        if (!require(
                centerline.surface == sim::TrackSurface::Asphalt && circularError < 0.1F,
                "Centerline pose/sample mismatch")) {
            return 1;
        }
    }

    const sim::TrackSample asphalt = track.sample(start.centerX, start.centerZ);
    const sim::TrackSample apron =
        track.sample(start.centerX - track.roadHalfWidthM() - 1.0F, start.centerZ);
    const sim::TrackSample grass =
        track.sample(start.centerX - track.roadHalfWidthM() - config.apronWidthM - 2.0F, start.centerZ);
    if (!require(asphalt.surface == sim::TrackSurface::Asphalt, "Asphalt classification failed") ||
        !require(apron.surface == sim::TrackSurface::Apron, "Apron classification failed") ||
        !require(grass.surface == sim::TrackSurface::Grass, "Grass classification failed")) {
        return 1;
    }
    if (!require(
            near(asphalt.longitudinalGripMultiplier, asphalt.gripMultiplier) &&
                grass.longitudinalGripMultiplier > grass.gripMultiplier * 2.0F,
            "Grass should keep low lateral grip but expose higher straight-line drive grip")) {
        return 1;
    }
    if (!require(
            track.innerBarrierOffsetM() < -track.roadHalfWidthM() - config.apronWidthM - 2.0F,
            "Inner wall should sit beyond a visible infield grass runout")) {
        return 1;
    }
    const float innerApronEdge = -track.roadHalfWidthM() - config.apronWidthM;
    const float infieldRunoutOffset = (innerApronEdge + track.innerBarrierOffsetM()) * 0.5F;
    const sim::TrackSample infieldRunout =
        track.sample(start.centerX + infieldRunoutOffset, start.centerZ);
    if (!require(
            infieldRunout.surface == sim::TrackSurface::Grass,
            "Infield runout before the inner wall should be grass")) {
        return 1;
    }

    const sim::TrackPose cornerApex = track.poseAtDistance(
        track.halfStraightLengthM() + std::numbers::pi_v<float> * config.cornerRadiusM * 0.25F);
    const sim::TrackSample innerApronAtApex = track.sample(
        cornerApex.centerX - cornerApex.rightX * (track.roadHalfWidthM() + config.apronWidthM),
        cornerApex.centerZ - cornerApex.rightZ * (track.roadHalfWidthM() + config.apronWidthM));
    if (!require(
            near(innerApronAtApex.heightM, 0.0F, 0.05F),
            "Banked inner apron should meet the ground plane")) {
        return 1;
    }

    sim::LapTimer timer(track.lapLengthM());
    timer.reset(track.lapLengthM() - 10.0F);
    timer.update(1.0F, true, 0.1F);
    if (!require(timer.state().active && timer.state().nextCheckpoint == 1, "Lap did not start")) {
        return 1;
    }
    timer.update(track.lapLengthM() * 0.25F + 1.0F, true, 10.0F);
    timer.update(track.lapLengthM() * 0.5F + 1.0F, true, 10.0F);
    timer.update(track.lapLengthM() * 0.75F + 1.0F, true, 10.0F);
    timer.update(track.lapLengthM() - 1.0F, true, 10.0F);
    timer.update(1.0F, true, 0.1F);
    if (!require(
            timer.state().completedLaps == 1 && timer.state().lastLapValid,
            "Valid checkpoint lap was not completed")) {
        return 1;
    }

    timer.update(track.lapLengthM() * 0.25F + 1.0F, false, 10.0F);
    if (!require(!timer.state().currentLapValid, "Off-track lap was not invalidated")) {
        return 1;
    }
    timer.resetRecords();
    if (!require(
            near(timer.state().bestLapSeconds, 0.0F) &&
                near(timer.state().lastLapSeconds, 0.0F) &&
                !timer.state().lastLapValid,
            "Lap record reset did not clear stored records")) {
        return 1;
    }

    sim::ConfigFile vehicleFile;
    if (!vehicleFile.load(std::string(argv[1]) + "/vehicle_openwheel_default.json")) {
        std::cerr << vehicleFile.error() << '\n';
        return 1;
    }
    sim::VehicleConfig vehicleConfig;
    vehicleConfig.load(vehicleFile);
    sim::Vehicle vehicle(vehicleConfig);
    sim::RaceSession race(config);
    race.resetVehicle(vehicle);
    if (!require(
            near(vehicle.current().positionX, config.shortChuteLengthM * 0.5F + config.cornerRadiusM),
            "Race reset did not place the car on the front straight")) {
        return 1;
    }
    const float grassRunoutOffset = -track.roadHalfWidthM() - config.apronWidthM - 4.0F;
    vehicle.reset(start.centerX + grassRunoutOffset, start.centerZ, start.yawRadians);
    sim::InputActions grassEscapeInput;
    grassEscapeInput.throttle = 0.72F;
    for (int step = 0; step < 720; ++step) {
        race.step(vehicle, grassEscapeInput, kPhysicsDt);
    }
    if (vehicle.current().speedMps <= 6.0F ||
        race.telemetry().surface != sim::TrackSurface::Grass ||
        race.telemetry().longitudinalGripMultiplier <= race.telemetry().gripMultiplier) {
        std::cerr << "Grass straight-line traction should let the car drive out instead of endless wheelspin"
                  << " speed=" << vehicle.current().speedMps
                  << " surface=" << sim::trackSurfaceName(race.telemetry().surface)
                  << " latGrip=" << race.telemetry().gripMultiplier
                  << " longGrip=" << race.telemetry().longitudinalGripMultiplier
                  << " rearSlip=" << vehicle.current().rearLongitudinalSlip << '\n';
        return 1;
    }

    vehicle.reset(start.centerX + track.outerWallOffsetM() + 2.0F, 0.0F, 0.0F);
    sim::InputActions noInput;
    race.step(vehicle, noInput, kPhysicsDt);
    const sim::TrackSample contained =
        track.sample(vehicle.current().positionX, vehicle.current().positionZ);
    if (!require(
            contained.lateralOffsetM <= track.outerWallOffsetM() + 0.01F &&
                race.telemetry().wallContact,
            "Outer wall did not contain the vehicle")) {
        return 1;
    }
    vehicle.reset(start.centerX + track.outerWallOffsetM() - vehicle.collisionHalfWidthM() * 0.35F, 0.0F, 0.0F);
    race.step(vehicle, noInput, kPhysicsDt);
    if (!require(race.telemetry().wallContact, "Outer wall did not catch vehicle corner penetration")) {
        return 1;
    }

    vehicle.reset(start.centerX + track.innerBarrierOffsetM() - 2.0F, 0.0F, 0.0F);
    race.step(vehicle, noInput, kPhysicsDt);
    const sim::TrackSample innerContained =
        track.sample(vehicle.current().positionX, vehicle.current().positionZ);
    if (!require(
            innerContained.lateralOffsetM >= track.innerBarrierOffsetM() - 0.01F &&
                race.telemetry().wallContact,
            "Inner infield barrier did not contain the vehicle")) {
        return 1;
    }
    vehicle.reset(start.centerX + track.innerBarrierOffsetM() + vehicle.collisionHalfWidthM() * 0.35F, 0.0F, 0.0F);
    race.step(vehicle, noInput, kPhysicsDt);
    if (!require(race.telemetry().wallContact, "Inner wall did not catch vehicle corner penetration")) {
        return 1;
    }

    return 0;
}
