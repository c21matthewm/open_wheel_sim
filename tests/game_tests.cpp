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
    sim::OvalTrack track(config);
    if (!require(track.config().type == "oval", "Track type config did not load oval")) {
        return 1;
    }
    if (!require(
            near(config.roadWidthM, 16.764F, 0.001F) &&
                near(config.straightRoadWidthM, 15.240F, 0.001F) &&
                near(config.turnRoadWidthM, 18.288F, 0.001F) &&
                near(config.apronWidthM, 6.096F, 0.001F) &&
                near(config.outerWallGapM, 0.610F, 0.001F) &&
                near(config.innerWallGapM, 9.144F, 0.001F),
            "IMS oval precision dimensions did not load")) {
        return 1;
    }

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
    for (const float checkpoint : {0.0F, track.lapLengthM() * 0.25F, track.lapLengthM() * 0.5F, track.lapLengthM() * 0.75F}) {
        if (!require(
                near(track.poseAtDistance(checkpoint).roadPitchRadians, 0.0F, 0.0001F),
                "Oval road pitch must remain flat after grade support")) {
            return 1;
        }
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
    constexpr float kIr18HalfTrackM = 1.943F * 0.5F;
    const sim::TrackSample bankedLeftWheel = track.sample(
        cornerApex.centerX - cornerApex.rightX * kIr18HalfTrackM,
        cornerApex.centerZ - cornerApex.rightZ * kIr18HalfTrackM);
    const sim::TrackSample bankedRightWheel = track.sample(
        cornerApex.centerX + cornerApex.rightX * kIr18HalfTrackM,
        cornerApex.centerZ + cornerApex.rightZ * kIr18HalfTrackM);
    if (!require(
            bankedRightWheel.heightM - bankedLeftWheel.heightM > 0.10F &&
                bankedLeftWheel.terrainHeightEnabled &&
                bankedRightWheel.terrainHeightEnabled,
            "Oval contact heights should differ across the banked tire track width")) {
        return 1;
    }
    sim::TrackConfig disabledTerrainConfig = config;
    disabledTerrainConfig.terrainHeightEnabled = false;
    sim::OvalTrack disabledTerrainTrack(disabledTerrainConfig);
    const sim::TrackSample disabledCenter =
        disabledTerrainTrack.sample(cornerApex.centerX, cornerApex.centerZ);
    if (!require(
            !disabledCenter.terrainHeightEnabled &&
                near(disabledCenter.heightM, cornerApex.centerHeightM, 0.001F) &&
                near(disabledCenter.surfaceRoughnessM, 0.0F) &&
                near(disabledCenter.curbHeightM, 0.0F),
            "Disabled terrain height must preserve the old analytic road plane")) {
        return 1;
    }

    sim::LapTimer timer(track.lapLengthM());
    const sim::TrackPose frontStraightCenter = track.poseAtDistance(0.0F);
    const sim::TrackPose startFinishPose = track.poseAtDistance(config.yardOfBricksPositionM);
    const float startFinishAlongStraight =
        (startFinishPose.centerX - frontStraightCenter.centerX) * frontStraightCenter.tangentX +
        (startFinishPose.centerZ - frontStraightCenter.centerZ) * frontStraightCenter.tangentZ;
    const float startFinishLateralOffset =
        (startFinishPose.centerX - frontStraightCenter.centerX) * frontStraightCenter.rightX +
        (startFinishPose.centerZ - frontStraightCenter.centerZ) * frontStraightCenter.rightZ;
    if (!require(
            near(config.yardOfBricksPositionM, 304.8F, 0.01F) &&
                near(config.checkpointsM.front(), config.yardOfBricksPositionM, 0.01F) &&
                near(startFinishAlongStraight, 304.8F, 0.01F) &&
                near(startFinishLateralOffset, 0.0F, 0.01F) &&
                config.yardOfBricksPositionM < track.halfStraightLengthM() &&
                track.checkpointState(305.8F).sector == 0 &&
                track.checkpointState(1311.0F).sector == 1 &&
                track.checkpointState(2317.0F).sector == 2 &&
                track.checkpointState(3323.0F).sector == 3,
            "Oval start/finish must be 304.8 m past front-straight center")) {
        return 1;
    }
    const float ovalStartFinishM = config.checkpointsM.front();
    timer.reset(track.checkpointState(ovalStartFinishM - 10.0F));
    timer.update(track.checkpointState(ovalStartFinishM + 1.0F), true, 0.1F);
    if (!require(timer.state().active && timer.state().nextCheckpoint == 1, "Lap did not start")) {
        return 1;
    }
    timer.update(track.checkpointState(1311.0F), true, 10.0F);
    timer.update(track.checkpointState(2317.0F), true, 10.0F);
    timer.update(track.checkpointState(3323.0F), true, 10.0F);
    timer.update(track.checkpointState(ovalStartFinishM + 1.0F), true, 10.0F);
    if (!require(
            timer.state().completedLaps == 1 && timer.state().lastLapValid,
            "Valid checkpoint lap was not completed")) {
        return 1;
    }

    timer.update(track.checkpointState(ovalStartFinishM + 100.0F), false, 10.0F);
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
    const sim::TrackPose expectedSpawn =
        track.poseAtDistance(ovalStartFinishM - config.spawnDistanceBeforeStartM);
    if (!require(
            near(vehicle.current().positionX, expectedSpawn.centerX) &&
                near(vehicle.current().positionZ, expectedSpawn.centerZ),
            "Race reset did not place the car 40 m before the Yard of Bricks")) {
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

    sim::InputActions noInput;
    sim::Vehicle straddleVehicle(vehicleConfig);
    sim::RaceSession straddleRace(config);
    const float straddleOffset = -track.roadHalfWidthM() + vehicleConfig.trackWidthM * 0.25F;
    straddleVehicle.reset(start.centerX + straddleOffset, start.centerZ, start.yawRadians);
    const sim::TrackSample straddleCenter =
        track.sample(straddleVehicle.current().positionX, straddleVehicle.current().positionZ);
    straddleRace.step(straddleVehicle, noInput, kPhysicsDt);
    if (!require(
            straddleCenter.surface == sim::TrackSurface::Asphalt &&
                straddleRace.telemetry().surface == sim::TrackSurface::Apron,
            "Per-wheel surface sampling should report apron when only inside tires leave asphalt")) {
        return 1;
    }

    sim::Vehicle grassBrushVehicle(vehicleConfig);
    sim::RaceSession grassBrushRace(config);
    grassBrushRace.resetVehicle(grassBrushVehicle);
    const float startLineDistance = config.checkpointsM.front();
    const sim::TrackPose justAfterStart = track.poseAtDistance(startLineDistance + 1.0F);
    grassBrushVehicle.reset(
        justAfterStart.centerX,
        justAfterStart.centerZ,
        justAfterStart.yawRadians);
    grassBrushRace.step(grassBrushVehicle, noInput, kPhysicsDt);
    const sim::TrackPose brushPose = track.poseAtDistance(startLineDistance + 5.0F);
    const float brushOffset = track.roadHalfWidthM() - vehicleConfig.trackWidthM * 0.25F;
    grassBrushVehicle.reset(
        brushPose.centerX + brushPose.rightX * brushOffset,
        brushPose.centerZ + brushPose.rightZ * brushOffset,
        brushPose.yawRadians);
    const sim::TrackSample brushCenter = track.sample(
        grassBrushVehicle.current().positionX,
        grassBrushVehicle.current().positionZ);
    grassBrushRace.step(grassBrushVehicle, noInput, kPhysicsDt);
    if (!require(
            brushCenter.surface == sim::TrackSurface::Asphalt &&
                grassBrushRace.telemetry().surface == sim::TrackSurface::Grass &&
                grassBrushRace.telemetry().lap.active &&
                grassBrushRace.telemetry().lap.currentLapValid,
            "A grass-touching tire must not invalidate a lap while the car center stays on asphalt")) {
        return 1;
    }

    vehicle.reset(start.centerX + track.outerWallOffsetM() + 2.0F, 0.0F, 0.0F);
    race.step(vehicle, noInput, kPhysicsDt);
    const sim::TrackSample contained =
        track.sample(vehicle.current().positionX, vehicle.current().positionZ);
    const sim::BarrierSample outerContained =
        track.outerBarrier(vehicle.current().positionX, vehicle.current().positionZ);
    if (!require(
            contained.lateralOffsetM <= track.outerWallOffsetM() + 0.01F &&
                outerContained.signedDistanceM >= -0.01F &&
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
    const sim::BarrierSample innerBarrierContained =
        track.innerBarrier(vehicle.current().positionX, vehicle.current().positionZ);
    if (!require(
            innerContained.lateralOffsetM >= track.innerBarrierOffsetM() - 0.01F &&
                innerBarrierContained.signedDistanceM >= -0.01F &&
                race.telemetry().wallContact,
            "Inner infield barrier did not contain the vehicle")) {
        return 1;
    }
    vehicle.reset(start.centerX + track.innerBarrierOffsetM() + vehicle.collisionHalfWidthM() * 0.35F, 0.0F, 0.0F);
    race.step(vehicle, noInput, kPhysicsDt);
    if (!require(race.telemetry().wallContact, "Inner wall did not catch vehicle corner penetration")) {
        return 1;
    }

    sim::ConfigFile hillConfigFile;
    if (!hillConfigFile.load(std::string(argv[1]) + "/track_hillcircuit_default.json")) {
        std::cerr << hillConfigFile.error() << '\n';
        return 1;
    }
    sim::TrackConfig hillConfig;
    hillConfig.load(hillConfigFile);
    sim::HillCircuitTrack hillTrack(hillConfig);
    if (!require(
            hillTrack.config().name == "Hill Circuit" &&
                hillTrack.config().type == "hillcircuit" &&
                near(hillTrack.lapLengthM(), 3600.0F),
            "Hill Circuit config did not load expected identity and lap length")) {
        return 1;
    }
    const sim::TrackPose hillStart = hillTrack.poseAtDistance(0.0F);
    const sim::TrackSample hillAsphalt = hillTrack.sample(hillStart.centerX, hillStart.centerZ);
    const sim::TrackSample hillCurb = hillTrack.sample(
        hillStart.centerX + hillStart.rightX * 7.0F,
        hillStart.centerZ + hillStart.rightZ * 7.0F);
    const sim::TrackSample hillGrass = hillTrack.sample(
        hillStart.centerX + hillStart.rightX * 13.0F,
        hillStart.centerZ + hillStart.rightZ * 13.0F);
    if (!require(
            hillAsphalt.surface == sim::TrackSurface::Asphalt &&
                hillCurb.surface == sim::TrackSurface::Curb &&
                hillGrass.surface == sim::TrackSurface::Grass,
            "Hill Circuit asphalt/curb/grass classification failed")) {
        return 1;
    }
    if (!require(
            hillAsphalt.curbHeightM <= 0.001F &&
                hillCurb.curbHeightM > 0.025F &&
                hillCurb.heightM > hillAsphalt.heightM + 0.025F,
            "Hill Circuit curb contact height should be raised above adjacent asphalt")) {
        return 1;
    }
    sim::Vehicle curbSideVehicle(vehicleConfig);
    const float curbStraddleOffset =
        hillTrack.roadHalfWidthM() - vehicleConfig.trackWidthM * 0.25F;
    curbSideVehicle.reset(
        hillStart.centerX + hillStart.rightX * curbStraddleOffset,
        hillStart.centerZ + hillStart.rightZ * curbStraddleOffset,
        hillStart.yawRadians);
    const auto curbPatches = curbSideVehicle.wheelContactPatchesWorld();
    const sim::TrackSample curbSideLeftFront =
        hillTrack.sample(curbPatches[0].x, curbPatches[0].z);
    const sim::TrackSample curbSideRightFront =
        hillTrack.sample(curbPatches[1].x, curbPatches[1].z);
    if (!require(
            curbSideLeftFront.surface == sim::TrackSurface::Asphalt &&
                curbSideLeftFront.curbHeightM <= 0.001F &&
                curbSideRightFront.surface == sim::TrackSurface::Curb &&
                curbSideRightFront.curbHeightM > 0.020F,
            "Only curb-side contact patches should receive raised curb height")) {
        return 1;
    }
    sim::HillCircuitTrack hintedHillTrack(hillConfig);
    for (const float distance : {120.0F, 580.0F, 1888.0F, 2620.0F, 3480.0F}) {
        const sim::TrackPose pose = hillTrack.poseAtDistance(distance);
        for (const float lateralOffset : {-7.0F, -1.0F, 1.0F, 7.0F}) {
            const float sampleX =
                pose.centerX + pose.rightX * lateralOffset + pose.tangentX * 1.7F;
            const float sampleZ =
                pose.centerZ + pose.rightZ * lateralOffset + pose.tangentZ * 1.7F;
            const sim::TrackSample exact = hillTrack.sample(sampleX, sampleZ);
            const sim::TrackSample hinted =
                hintedHillTrack.sampleNearDistance(sampleX, sampleZ, distance);
            if (!require(
                    hinted.surface == exact.surface &&
                        near(hinted.distanceM, exact.distanceM, 0.01F) &&
                        near(hinted.lateralOffsetM, exact.lateralOffsetM, 0.01F),
                    "Hill Circuit distance-hinted wheel sample diverged from exact query")) {
                return 1;
            }
        }
    }
    if (!require(
            hillCurb.gripMultiplier < hillAsphalt.gripMultiplier &&
                hillCurb.longitudinalGripMultiplier < hillAsphalt.longitudinalGripMultiplier &&
                hillCurb.resistanceMultiplier > hillAsphalt.resistanceMultiplier,
            "Hill Circuit curb physics multipliers should reduce grip and add rumble drag")) {
        return 1;
    }
    const sim::TrackPose hillHighPoint = hillTrack.poseAtDistance(1780.0F);
    const sim::TrackPose hillCorkscrew = hillTrack.poseAtDistance(1890.0F);
    if (!require(
            hillHighPoint.centerHeightM > 21.0F &&
                hillCorkscrew.centerHeightM < 14.0F &&
                hillCorkscrew.roadPitchRadians < -0.10F,
            "Hill Circuit corkscrew should crest high and descend steeply")) {
        return 1;
    }
    const float cachedSlope = hillTrack.slopeAtDistance(1888.0F);
    for (int sampleIndex = 0; sampleIndex < 1000; ++sampleIndex) {
        if (!require(
                hillTrack.slopeAtDistance(1888.0F) == cachedSlope,
                "Hill Circuit cached slope lookup changed within one segment")) {
            return 1;
        }
    }
    sim::LapTimer hillTimer(hillTrack.lapLengthM());
    hillTimer.reset(hillTrack.checkpointState(3590.0F));
    hillTimer.update(hillTrack.checkpointState(1.0F), true, 0.1F);
    hillTimer.update(hillTrack.checkpointState(621.0F), true, 20.0F);
    hillTimer.update(hillTrack.checkpointState(1301.0F), true, 20.0F);
    hillTimer.update(hillTrack.checkpointState(1961.0F), true, 20.0F);
    hillTimer.update(hillTrack.checkpointState(2561.0F), true, 20.0F);
    hillTimer.update(hillTrack.checkpointState(1.0F), true, 20.0F);
    if (!require(
            hillTimer.state().completedLaps == 1 && hillTimer.state().lastLapValid,
            "Hill Circuit checkpoint lap did not complete")) {
        return 1;
    }
    sim::RaceSession hillRace(hillConfig);
    sim::Vehicle hillVehicle(vehicleConfig);
    hillRace.resetVehicle(hillVehicle);
    const sim::TrackPose hillSpawn = hillTrack.spawnPose();
    if (!require(
            near(hillVehicle.current().positionX, hillSpawn.centerX, 0.1F) &&
                near(hillVehicle.current().positionZ, hillSpawn.centerZ, 0.1F),
            "Hill Circuit race reset did not use track spawn pose")) {
        return 1;
    }

    return 0;
}
