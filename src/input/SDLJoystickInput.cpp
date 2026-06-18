#include "input/SDLJoystickInput.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace sim {

SDLJoystickInput::~SDLJoystickInput() {
    shutdown();
}

void SDLJoystickInput::initialize() {
    refreshDevices();
}

void SDLJoystickInput::shutdown() {
    for (int index = 0; index < deviceCount_; ++index) {
        if (devices_[index].handle != nullptr) {
            SDL_CloseJoystick(devices_[index].handle);
        }
        devices_[index] = {};
    }
    deviceCount_ = 0;
}

void SDLJoystickInput::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED) {
        refreshPending_ = true;
    }
}

void SDLJoystickInput::update() {
    if (refreshPending_) {
        refreshDevices();
        refreshPending_ = false;
    }

    SDL_UpdateJoysticks();
    for (int deviceIndex = 0; deviceIndex < deviceCount_; ++deviceIndex) {
        Device& current = devices_[deviceIndex];
        if (current.handle == nullptr || !SDL_JoystickConnected(current.handle)) {
            continue;
        }
        for (int axis = 0; axis < current.axisCount; ++axis) {
            const Sint16 raw = SDL_GetJoystickAxis(current.handle, axis);
            current.axes[axis] =
                raw < 0 ? static_cast<float>(raw) / 32768.0F : static_cast<float>(raw) / 32767.0F;
        }
        for (int button = 0; button < current.buttonCount; ++button) {
            current.buttons[button] = SDL_GetJoystickButton(current.handle, button);
        }
        for (int hat = 0; hat < current.hatCount; ++hat) {
            current.hats[hat] = SDL_GetJoystickHat(current.handle, hat);
        }
    }
}

const SDLJoystickInput::Device* SDLJoystickInput::firstMatching(std::string_view nameFragment) const {
    for (int index = 0; index < deviceCount_; ++index) {
        const std::string_view name(devices_[index].name.data());
        if (nameFragment.empty() || name.find(nameFragment) != std::string_view::npos) {
            return &devices_[index];
        }
    }
    return nullptr;
}

void SDLJoystickInput::refreshDevices() {
    shutdown();

    int joystickCount = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&joystickCount);
    if (ids == nullptr) {
        SDL_Log("Could not enumerate joysticks: %s", SDL_GetError());
        return;
    }

    const int openCount = std::min(joystickCount, kMaxDevices);
    for (int index = 0; index < openCount; ++index) {
        SDL_Joystick* joystick = SDL_OpenJoystick(ids[index]);
        if (joystick == nullptr) {
            SDL_Log("Could not open joystick %u: %s", ids[index], SDL_GetError());
            continue;
        }

        Device& current = devices_[deviceCount_++];
        current.handle = joystick;
        current.id = ids[index];
        const char* name = SDL_GetJoystickName(joystick);
        std::snprintf(current.name.data(), current.name.size(), "%s", name != nullptr ? name : "Unknown");
        current.vendor = SDL_GetJoystickVendor(joystick);
        current.product = SDL_GetJoystickProduct(joystick);
        current.axisCount = std::min(SDL_GetNumJoystickAxes(joystick), kMaxAxes);
        current.buttonCount = std::min(SDL_GetNumJoystickButtons(joystick), kMaxButtons);
        current.hatCount = std::min(SDL_GetNumJoystickHats(joystick), kMaxHats);
        current.isGamepad = SDL_IsGamepad(ids[index]);
        current.isHaptic = SDL_IsJoystickHaptic(joystick);
    }

    SDL_free(ids);
}

}  // namespace sim
