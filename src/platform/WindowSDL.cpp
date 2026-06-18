#include "platform/WindowSDL.h"

namespace sim {

WindowSDL::~WindowSDL() {
    shutdown();
}

bool WindowSDL::initialize(const GraphicsConfig& config) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        error_ = SDL_GetError();
        return false;
    }

    SDL_WindowFlags flags = SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.borderless) {
        flags |= SDL_WINDOW_BORDERLESS;
    }
    if (config.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
    if (window_ == nullptr) {
        error_ = SDL_GetError();
        SDL_Quit();
        return false;
    }

    fullscreen_ = config.fullscreen;
    return true;
}

void WindowSDL::shutdown() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void WindowSDL::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN) {
        fullscreen_ = true;
    } else if (event.type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN) {
        fullscreen_ = false;
    }
}

void WindowSDL::toggleFullscreen() {
    if (window_ == nullptr) {
        return;
    }
    if (!SDL_SetWindowFullscreen(window_, !fullscreen_)) {
        SDL_Log("Could not toggle fullscreen: %s", SDL_GetError());
        return;
    }
    fullscreen_ = !fullscreen_;
}

int WindowSDL::pixelWidth() const {
    int width = 0;
    int height = 0;
    if (window_ != nullptr) {
        SDL_GetWindowSizeInPixels(window_, &width, &height);
    }
    return width;
}

int WindowSDL::pixelHeight() const {
    int width = 0;
    int height = 0;
    if (window_ != nullptr) {
        SDL_GetWindowSizeInPixels(window_, &width, &height);
    }
    return height;
}

}  // namespace sim
