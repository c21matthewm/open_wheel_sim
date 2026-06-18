#pragma once

#include <array>
#include <cstdio>

#include "input/InputActions.h"
#include "input/SDLJoystickInput.h"
#include "game/RaceSession.h"
#include "physics/Vehicle.h"
#include "render/RenderTypes.h"

namespace sim {

struct OverlayColor {
    float red = 1.0F;
    float green = 1.0F;
    float blue = 1.0F;
    float alpha = 1.0F;
};

struct OverlayLine {
    float x = 0.0F;
    float y = 0.0F;
    float scale = 1.0F;
    OverlayColor color;
    std::array<char, 128> text{};
};

struct MenuOverlayState {
    bool visible = false;
    int selectedIndex = 0;
    float keyboardSteerRate = 0.0F;
    float keyboardReturnRate = 0.0F;
    float brakeBias = 0.0F;
    bool automaticShift = false;
    bool ghostAvailable = false;
    const char* aeroPresetName = "SPEEDWAY";
};

class DebugOverlay {
public:
    static constexpr int kMaxLines = 64;

    explicit DebugOverlay(bool diagnosticsVisible);

    void toggleVisible() { visible_ = !visible_; }
    void toggleDiagnostics() { diagnosticsVisible_ = !diagnosticsVisible_; }
    void build(
        const PerformanceStats& stats,
        const VehicleState& vehicle,
        const RaceTelemetry& race,
        const InputActions& input,
        const SDLJoystickInput& joysticks,
        int pixelWidth,
        int pixelHeight,
        const MenuOverlayState& menu,
        const char* ffbStatus);

    [[nodiscard]] bool visible() const { return visible_; }
    [[nodiscard]] int lineCount() const { return lineCount_; }
    [[nodiscard]] const OverlayLine& line(int index) const { return lines_[index]; }

private:
    template <typename... Args>
    void add(float x, float y, OverlayColor color, const char* format, Args... args) {
        if (lineCount_ >= kMaxLines) {
            return;
        }
        OverlayLine& line = lines_[lineCount_++];
        line.x = x;
        line.y = y;
        line.scale = 1.0F;
        line.color = color;
        if constexpr (sizeof...(args) == 0) {
            std::snprintf(line.text.data(), line.text.size(), "%s", format);
        } else {
            std::snprintf(line.text.data(), line.text.size(), format, args...);
        }
    }

    template <typename... Args>
    void addScaled(float x, float y, float scale, OverlayColor color, const char* format, Args... args) {
        if (lineCount_ >= kMaxLines) {
            return;
        }
        OverlayLine& line = lines_[lineCount_++];
        line.x = x;
        line.y = y;
        line.scale = scale;
        line.color = color;
        if constexpr (sizeof...(args) == 0) {
            std::snprintf(line.text.data(), line.text.size(), "%s", format);
        } else {
            std::snprintf(line.text.data(), line.text.size(), format, args...);
        }
    }

    std::array<OverlayLine, kMaxLines> lines_{};
    int lineCount_ = 0;
    bool visible_ = true;
    bool diagnosticsVisible_ = true;
};

}  // namespace sim
