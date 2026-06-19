#pragma once

#include "game/Track.h"
#include "physics/Vehicle.h"

namespace sim {

struct PerformanceStats {
    float fps = 0.0F;
    float frameMs = 0.0F;
    float inputMs = 0.0F;
    float physicsMs = 0.0F;
    float renderMs = 0.0F;
    int physicsSteps = 0;
};

struct RenderGhostPose {
    bool available = false;
    float positionX = 0.0F;
    float positionZ = 0.0F;
    float yawRadians = 0.0F;
    float heightM = 0.0F;
    float bankRadians = 0.0F;
};

struct RenderScene {
    VehicleState vehicle;
    float vehicleHeightM = 0.0F;
    float vehicleBankRadians = 0.0F;
    RenderGhostPose ghost;
    float brakeInput = 0.0F;
    float throttleInput = 0.0F;
    TrackSurface surface = TrackSurface::Asphalt;
    bool wallContact = false;
    int pixelWidth = 1280;
    int pixelHeight = 720;
    float currentLapSeconds = 0.0F;
    float bestLapSeconds = 0.0F;
    float lapDeltaSeconds = 0.0F;
    int cameraMode = 0;
};

struct HomeScreenState {
    int pixelWidth = 1280;
    int pixelHeight = 720;
    int selectedItem = 0;
    int selectedTrack = 0;
    float frontWingSetting = 0.0F;
    float rearWingSetting = 0.0F;
    float wingSettingMin = -3.0F;
    float wingSettingMax = 3.0F;
    float brakeBias = 0.58F;
};

}  // namespace sim
