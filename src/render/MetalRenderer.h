#pragma once

#include <memory>
#include <string>

#include "render/Renderer.h"

namespace sim {

class MetalRenderer final : public Renderer {
public:
    MetalRenderer();
    ~MetalRenderer() override;

    bool initialize(
        WindowSDL& window,
        bool vsync,
        float renderScale,
        int shadowMapSize,
        int shadowUpdateInterval,
        int msaaSamples,
        float shadowFrustumExtentM,
        float shadowLightDistanceM,
        float shadowLightHeightOffsetM,
        float bloomHalfWeight,
        float bloomQuarterWeight,
        float hudGlassBlurRadiusPx,
        float hudGlassRefractionRadiusPx,
        float cameraStartupShakeSuppressionS,
        float cameraStartupShakeFadeS,
        float cameraAsphaltShake,
        float cameraApronShake,
        float cameraGrassShake,
        float cameraRpmShake,
        float cameraSpeedShake,
        float cameraSlipStartUsage,
        float cameraSlipShakeUsageRange,
        float cameraSlipShake,
        float cameraTraumaShake,
        float cameraTraumaDecay,
        float cameraBankRollScale,
        float cameraLateralGRollScale,
        float cameraLongitudinalGRollScale,
        const Track& track) override;
    void shutdown() override;
    bool render(const RenderScene& scene, const DebugOverlay& overlay) override;
    [[nodiscard]] const std::string& error() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace sim
