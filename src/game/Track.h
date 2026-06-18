#pragma once

#include <string>

namespace sim {

class ConfigFile;

enum class TrackSurface {
    Asphalt,
    Apron,
    Grass,
};

[[nodiscard]] const char* trackSurfaceName(TrackSurface surface);

struct TrackConfig {
    std::string name = "2.5-Mile Test Oval";
    float straightLengthM = 1005.84F;
    float shortChuteLengthM = 201.168F;
    float cornerRadiusM = 256.135F;
    float roadWidthM = 16.0F;
    float apronWidthM = 8.0F;
    float outerWallGapM = 1.5F;
    float innerWallGapM = 12.0F;
    float outerWallHeightM = 1.2F;
    float bankingDegrees = 9.2F;
    float bankTransitionFraction = 0.12F;
    float bankTransitionRunoutM = 50.0F;
    float spawnDistanceBeforeStartM = 25.0F;
    float asphaltGripMultiplier = 1.0F;
    float apronGripMultiplier = 0.88F;
    float grassGripMultiplier = 0.38F;
    float grassLongitudinalGripMultiplier = 1.45F;
    float asphaltResistanceMultiplier = 1.0F;
    float apronResistanceMultiplier = 1.5F;
    float grassResistanceMultiplier = 8.0F;
    float wallRestitution = 0.12F;
    int renderSegments = 512;

    void load(const ConfigFile& config);
};

struct TrackPose {
    float centerX = 0.0F;
    float centerZ = 0.0F;
    float tangentX = 0.0F;
    float tangentZ = 1.0F;
    float rightX = 1.0F;
    float rightZ = 0.0F;
    float yawRadians = 0.0F;
    float bankRadians = 0.0F;
    float centerHeightM = 0.0F;
    float distanceM = 0.0F;
    float roadHalfWidthM = 8.0F;
};

struct TrackSample : TrackPose {
    float lateralOffsetM = 0.0F;
    float heightM = 0.0F;
    float gripMultiplier = 1.0F;
    float longitudinalGripMultiplier = 1.0F;
    float resistanceMultiplier = 1.0F;
    TrackSurface surface = TrackSurface::Asphalt;
};

class Track {
public:
    explicit Track(TrackConfig config);

    [[nodiscard]] const TrackConfig& config() const { return config_; }
    [[nodiscard]] float lapLengthM() const { return lapLengthM_; }
    [[nodiscard]] float halfStraightLengthM() const { return halfStraightLengthM_; }
    [[nodiscard]] float roadHalfWidthM() const { return config_.roadWidthM * 0.5F; }
    [[nodiscard]] float outerWallOffsetM() const {
        return roadHalfWidthM() + config_.outerWallGapM;
    }
    [[nodiscard]] float innerBarrierOffsetM() const {
        return -(roadHalfWidthM() + config_.apronWidthM + config_.innerWallGapM);
    }

    [[nodiscard]] TrackPose poseAtDistance(float distanceM) const;
    [[nodiscard]] TrackPose spawnPose() const;
    [[nodiscard]] TrackSample sample(float positionX, float positionZ) const;

private:
    [[nodiscard]] float bankForDistance(float distanceM) const;
    [[nodiscard]] float normalizeDistance(float distanceM) const;

    TrackConfig config_;
    float halfStraightLengthM_ = 0.0F;
    float lapLengthM_ = 0.0F;
};

}  // namespace sim
