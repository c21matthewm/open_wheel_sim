#pragma once

#include <string>

#include "render/RenderTypes.h"

namespace sim {

class DebugOverlay;
class Track;
class WindowSDL;

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool initialize(
        WindowSDL& window,
        bool vsync,
        float renderScale,
        int shadowMapSize,
        int shadowUpdateInterval,
        int msaaSamples,
        const Track& track) = 0;
    virtual void shutdown() = 0;
    virtual bool render(const RenderScene& scene, const DebugOverlay& overlay) = 0;
    [[nodiscard]] virtual const std::string& error() const = 0;
};

}  // namespace sim
