#pragma once

#include <string>

#include <SDL3/SDL.h>

#include "config/AppConfig.h"

namespace sim {

class WindowSDL {
public:
    WindowSDL() = default;
    ~WindowSDL();

    WindowSDL(const WindowSDL&) = delete;
    WindowSDL& operator=(const WindowSDL&) = delete;

    bool initialize(const GraphicsConfig& config);
    void shutdown();
    void handleEvent(const SDL_Event& event);
    void toggleFullscreen();

    [[nodiscard]] SDL_Window* nativeHandle() const { return window_; }
    [[nodiscard]] bool isFullscreen() const { return fullscreen_; }
    [[nodiscard]] int pixelWidth() const;
    [[nodiscard]] int pixelHeight() const;
    [[nodiscard]] const std::string& error() const { return error_; }

private:
    SDL_Window* window_ = nullptr;
    bool fullscreen_ = false;
    std::string error_;
};

}  // namespace sim
