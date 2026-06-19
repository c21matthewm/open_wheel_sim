#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace sim {

class ConfigFile;

enum class SurfaceType {
    Asphalt,
    Apron,
    Grass,
    Curb,
};

using TrackSurface = SurfaceType;

[[nodiscard]] const char* trackSurfaceName(TrackSurface surface);

struct TrackConfig {
    std::string name = "2.5-Mile Test Oval";
    std::string type = "oval";
    float straightLengthM = 1005.84F;
    float shortChuteLengthM = 201.168F;
    float cornerRadiusM = 256.135F;
    float roadWidthM = 16.0F;
    float straightRoadWidthM = 0.0F;
    float turnRoadWidthM = 0.0F;
    float apronWidthM = 8.0F;
    float curbWidthM = 0.0F;
    float runoffWidthM = 0.0F;
    float outerWallGapM = 1.5F;
    float innerWallGapM = 12.0F;
    float barrierOuterOffsetM = 1.0F;
    float barrierInnerOffsetM = 1.0F;
    float outerWallHeightM = 1.2F;
    float bankingDegrees = 9.2F;
    float bankTransitionFraction = 0.12F;
    float bankTransitionRunoutM = 50.0F;
    float pitLaneOffsetM = 11.0F;
    float pitLaneWidthM = 7.925F;
    float pitLaneStartM = 20.0F;
    float pitLaneEndM = 422.0F;
    int pitBoxCount = 33;
    float pitBoxDepthM = 4.267F;
    float pitWallHeightM = 1.067F;
    float yardOfBricksWidthM = 0.914F;
    float yardOfBricksPositionM = 0.0F;
    float spawnDistanceBeforeStartM = 25.0F;
    float spawnDistanceM = 0.0F;
    float spawnHeadingOffsetRadians = 0.0F;
    float lapLengthOverrideM = 0.0F;
    float asphaltGripMultiplier = 1.0F;
    float apronGripMultiplier = 0.88F;
    float curbGripMultiplier = 0.85F;
    float curbLongitudinalGripMultiplier = 0.80F;
    float curbResistanceMultiplier = 1.8F;
    float grassGripMultiplier = 0.38F;
    float grassLongitudinalGripMultiplier = 1.45F;
    float asphaltResistanceMultiplier = 1.0F;
    float apronResistanceMultiplier = 1.5F;
    float grassResistanceMultiplier = 8.0F;
    bool terrainHeightEnabled = true;
    float asphaltRoughnessM = 0.004F;
    float ovalSeamAmpM = 0.008F;
    float ovalUndulationAmpM = 0.006F;
    float curbHeightM = 0.045F;
    float contactHeightFilterTauS = 0.035F;
    float wallRestitution = 0.12F;
    float wallFriction = 0.55F;
    int renderSegments = 512;
    std::vector<float> checkpointsM;
    std::vector<std::array<float, 2>> elevationSpine;

    void load(const ConfigFile& config);
};

struct TrackPose {
    union {
        float centerX = 0.0F;
        float x;
    };
    union {
        float centerZ = 0.0F;
        float z;
    };
    union {
        float tangentX = 0.0F;
        float tangent_x;
    };
    union {
        float tangentZ = 1.0F;
        float tangent_z;
    };
    float rightX = 1.0F;
    float rightZ = 0.0F;
    union {
        float yawRadians = 0.0F;
        float heading;
    };
    union {
        float bankRadians = 0.0F;
        float bank;
    };
    union {
        float centerHeightM = 0.0F;
        float elevation;
    };
    float roadPitchRadians = 0.0F;
    float distanceM = 0.0F;
    float roadHalfWidthM = 8.0F;
};

struct TrackSample : TrackPose {
    float lateralOffsetM = 0.0F;
    union {
        float heightM = 0.0F;
        float road_height_y_m;
    };
    float roadNormalX = 0.0F;
    float roadNormalY = 1.0F;
    float roadNormalZ = 0.0F;
    float surfaceRoughnessM = 0.0F;
    float curbHeightM = 0.0F;
    float curbPhase = 0.0F;
    float contactHeightFilterTauS = 0.0F;
    float smoothHeightM = 0.0F;
    bool terrainHeightEnabled = false;
    union {
        float gripMultiplier = 1.0F;
        float lateral_grip;
    };
    union {
        float longitudinalGripMultiplier = 1.0F;
        float longitudinal_grip;
    };
    union {
        float resistanceMultiplier = 1.0F;
        float rolling_resistance;
    };
    TrackSurface surface = TrackSurface::Asphalt;
};

struct CheckpointState {
    float distanceM = 0.0F;
    int sector = 0;
    int checkpointCount = 4;
    int next_checkpoint = 0;
    bool lap_valid = true;
    float startFinishDistanceM = 0.0F;
};

struct BarrierSample {
    float signedDistanceM = 0.0F;
    union {
        float normalX = 0.0F;
        float normal_x;
    };
    union {
        float normalZ = 0.0F;
        float normal_z;
    };
    float restitution = 0.0F;
    float x = 0.0F;
    float z = 0.0F;
};

class Track {
public:
    virtual ~Track() = default;

    [[nodiscard]] virtual TrackPose poseAtDistance(float distanceM) const = 0;
    [[nodiscard]] virtual TrackSample sample(float positionX, float positionZ) const = 0;
    [[nodiscard]] virtual TrackSample sampleNearDistance(
        float positionX,
        float positionZ,
        float distanceHintM) const {
        static_cast<void>(distanceHintM);
        return sample(positionX, positionZ);
    }
    [[nodiscard]] virtual TrackPose spawnPose() const = 0;
    [[nodiscard]] virtual float lapLengthM() const = 0;
    [[nodiscard]] virtual CheckpointState checkpointState(float distanceM) const = 0;
    [[nodiscard]] virtual BarrierSample outerBarrier(float positionX, float positionZ) const = 0;
    [[nodiscard]] virtual BarrierSample innerBarrier(float positionX, float positionZ) const = 0;
};

class OvalTrack final : public Track {
public:
    explicit OvalTrack(TrackConfig config);

    [[nodiscard]] const TrackConfig& config() const { return config_; }
    [[nodiscard]] float lapLengthM() const override { return lapLengthM_; }
    [[nodiscard]] float halfStraightLengthM() const { return halfStraightLengthM_; }
    [[nodiscard]] float roadHalfWidthM() const { return config_.roadWidthM * 0.5F; }
    [[nodiscard]] float outerWallOffsetM() const {
        return roadHalfWidthM() + config_.outerWallGapM;
    }
    [[nodiscard]] float innerBarrierOffsetM() const {
        return -(roadHalfWidthM() + config_.apronWidthM + config_.innerWallGapM);
    }

    [[nodiscard]] TrackPose poseAtDistance(float distanceM) const override;
    [[nodiscard]] TrackPose spawnPose() const override;
    [[nodiscard]] TrackSample sample(float positionX, float positionZ) const override;
    [[nodiscard]] CheckpointState checkpointState(float distanceM) const override;
    [[nodiscard]] BarrierSample outerBarrier(float positionX, float positionZ) const override;
    [[nodiscard]] BarrierSample innerBarrier(float positionX, float positionZ) const override;

private:
    [[nodiscard]] float bankForDistance(float distanceM) const;
    [[nodiscard]] float normalizeDistance(float distanceM) const;

    TrackConfig config_;
    float halfStraightLengthM_ = 0.0F;
    float lapLengthM_ = 0.0F;
};

class HillCircuitTrack final : public Track {
public:
    explicit HillCircuitTrack(TrackConfig config);

    [[nodiscard]] const TrackConfig& config() const { return config_; }
    [[nodiscard]] TrackPose poseAtDistance(float distanceM) const override;
    [[nodiscard]] TrackPose spawnPose() const override;
    [[nodiscard]] TrackSample sample(float positionX, float positionZ) const override;
    [[nodiscard]] TrackSample sampleNearDistance(
        float positionX,
        float positionZ,
        float distanceHintM) const override;
    [[nodiscard]] float lapLengthM() const override { return lapLengthM_; }
    [[nodiscard]] CheckpointState checkpointState(float distanceM) const override;
    [[nodiscard]] BarrierSample outerBarrier(float positionX, float positionZ) const override;
    [[nodiscard]] BarrierSample innerBarrier(float positionX, float positionZ) const override;
    [[nodiscard]] float roadHalfWidthM() const { return config_.roadWidthM * 0.5F; }
    [[nodiscard]] float slopeAtDistance(float distanceM) const;
    [[nodiscard]] float curbOuterOffsetM() const { return roadHalfWidthM() + config_.curbWidthM; }
    [[nodiscard]] float barrierOffsetM() const {
        return roadHalfWidthM() + config_.curbWidthM + config_.runoffWidthM + config_.barrierOuterOffsetM;
    }

private:
    struct CenterlinePoint {
        float distanceM = 0.0F;
        float x = 0.0F;
        float z = 0.0F;
        float tangentX = 0.0F;
        float tangentZ = 1.0F;
        float rightX = 1.0F;
        float rightZ = 0.0F;
        float heading = 0.0F;
        float elevationM = 0.0F;
        float roadPitchRadians = 0.0F;
        float bankRadians = 0.0F;
    };

    struct CenterlineChunkBounds {
        float minimumX = 0.0F;
        float maximumX = 0.0F;
        float minimumZ = 0.0F;
        float maximumZ = 0.0F;
        std::size_t firstSegment = 0U;
        std::size_t segmentCount = 0U;
    };

    [[nodiscard]] float normalizeDistance(float distanceM) const;
    [[nodiscard]] float elevationAtDistance(float distanceM) const;
    [[nodiscard]] float camberAtDistance(float distanceM) const;
    [[nodiscard]] CenterlinePoint interpolatedCenterline(float distanceM) const;
    [[nodiscard]] TrackSample nearestSample(
        float positionX,
        float positionZ,
        float distanceHintM,
        bool constrainToHint) const;
    void buildCenterline();

    TrackConfig config_;
    float lapLengthM_ = 3600.0F;
    static constexpr std::size_t kMaxElevationSpinePoints = 64U;
    std::array<float, kMaxElevationSpinePoints> elevationSlopes_{};
    std::size_t elevationSlopeCount_ = 0U;
    mutable int cachedSlopeSegment_ = 0;
    mutable float cachedSlopeDistanceM_ = -1.0F;
    mutable float cachedSlopeValue_ = 0.0F;
    mutable int cachedNearestSegment_ = -1;
    mutable float cachedNearestPositionX_ = 0.0F;
    mutable float cachedNearestPositionZ_ = 0.0F;
    mutable TrackSample cachedNearestSample_{};
    mutable bool cachedNearestSampleValid_ = false;
    static constexpr std::size_t kCenterlineChunkSize = 16U;
    static constexpr std::size_t kMaxCenterlineChunks = 32U;
    std::array<CenterlineChunkBounds, kMaxCenterlineChunks> centerlineChunkBounds_{};
    std::size_t centerlineChunkCount_ = 0U;
    std::vector<CenterlinePoint> centerline_;
};

[[nodiscard]] std::unique_ptr<Track> makeTrack(TrackConfig config);

}  // namespace sim
