#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <SDL3/SDL.h>

namespace sim {

class SDLJoystickInput {
public:
    static constexpr int kMaxDevices = 8;
    static constexpr int kMaxAxes = 16;
    static constexpr int kMaxButtons = 64;
    static constexpr int kMaxHats = 8;

    struct Device {
        SDL_Joystick* handle = nullptr;
        SDL_JoystickID id = 0;
        std::array<char, 128> name{};
        Uint16 vendor = 0;
        Uint16 product = 0;
        int axisCount = 0;
        int buttonCount = 0;
        int hatCount = 0;
        bool isGamepad = false;
        bool isHaptic = false;
        std::array<float, kMaxAxes> axes{};
        std::array<bool, kMaxButtons> buttons{};
        std::array<Uint8, kMaxHats> hats{};
    };

    ~SDLJoystickInput();

    void initialize();
    void shutdown();
    void handleEvent(const SDL_Event& event);
    void update();

    [[nodiscard]] int deviceCount() const { return deviceCount_; }
    [[nodiscard]] const Device& device(int index) const { return devices_[index]; }
    [[nodiscard]] const Device* firstMatching(std::string_view nameFragment) const;

private:
    void refreshDevices();

    std::array<Device, kMaxDevices> devices_{};
    int deviceCount_ = 0;
    bool refreshPending_ = false;
};

}  // namespace sim
