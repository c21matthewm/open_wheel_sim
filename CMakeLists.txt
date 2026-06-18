cmake_minimum_required(VERSION 3.24)

project(LightweightOpenWheelSim VERSION 0.1.0 LANGUAGES CXX OBJCXX)

if(NOT APPLE)
    message(FATAL_ERROR "The first prototype currently targets macOS and Metal only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_XCODE_GENERATE_SCHEME ON)

find_package(SDL3 CONFIG REQUIRED)

# Some Command Line Tools installs expose libc++ only inside the selected SDK
# while Apple Clang still points at a missing toolchain-level include directory.
set(LIGHTWEIGHT_SIM_HAS_LIBCXX_HEADERS FALSE)
foreach(IMPLICIT_INCLUDE_DIR IN LISTS CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
    if(EXISTS "${IMPLICIT_INCLUDE_DIR}/string")
        set(LIGHTWEIGHT_SIM_HAS_LIBCXX_HEADERS TRUE)
        break()
    endif()
endforeach()
if(NOT LIGHTWEIGHT_SIM_HAS_LIBCXX_HEADERS)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE LIGHTWEIGHT_SIM_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(EXISTS "${LIGHTWEIGHT_SIM_SDK_PATH}/usr/include/c++/v1/string")
        message(WARNING "Apple Clang's default libc++ include path is missing; using the selected SDK fallback.")
        include_directories(SYSTEM BEFORE "${LIGHTWEIGHT_SIM_SDK_PATH}/usr/include/c++/v1")
    endif()
endif()

set(LIGHTWEIGHT_SIM_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/app/GameLoop.cpp
    src/audio/SoundManager.cpp
    src/config/ConfigFile.cpp
    src/config/AppConfig.cpp
    src/ffb/ForceFeedbackManager.cpp
    src/game/LapTimer.cpp
    src/game/RaceSession.cpp
    src/game/Track.cpp
    src/input/InputManager.cpp
    src/input/SDLJoystickInput.cpp
    src/physics/Vehicle.cpp
    src/physics/VehicleConfig.cpp
    src/platform/FileSystem.cpp
    src/platform/WindowSDL.cpp
    src/render/MetalRenderer.mm
    src/ui/DebugOverlay.cpp
)

add_executable(LightweightSim MACOSX_BUNDLE ${LIGHTWEIGHT_SIM_SOURCES})

target_include_directories(LightweightSim PRIVATE src)
target_link_libraries(LightweightSim PRIVATE
    SDL3::SDL3
    "-framework Cocoa"
    "-framework CoreGraphics"
    "-framework ImageIO"
    "-framework Metal"
    "-framework QuartzCore"
)

target_compile_options(LightweightSim PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>
    $<$<COMPILE_LANGUAGE:OBJCXX>:-Wall;-Wextra;-Wpedantic;-fobjc-arc>
)

set_target_properties(LightweightSim PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "LightweightSim"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.lightweightsim.prototype"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    BUILD_RPATH "@executable_path/../Frameworks"
    INSTALL_RPATH "@executable_path/../Frameworks"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "dev.lightweightsim.prototype"
)

add_custom_command(TARGET LightweightSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3-shared>"
        "$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Frameworks/libSDL3.0.dylib"
    COMMAND ${CMAKE_COMMAND}
        "-DAPP_EXECUTABLE=$<TARGET_FILE:LightweightSim>"
        "-DSDL_SOURCE=$<TARGET_FILE:SDL3::SDL3-shared>"
        "-DSDL_BUNDLED=$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Frameworks/libSDL3.0.dylib"
        -P "${CMAKE_SOURCE_DIR}/cmake/BundleSDL3.cmake"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config"
        "$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Resources/config"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Resources/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/README.md"
        "$<TARGET_BUNDLE_CONTENT_DIR:LightweightSim>/Resources/README.txt"
    COMMAND /usr/bin/codesign --force --deep --sign -
        "$<TARGET_BUNDLE_DIR:LightweightSim>"
    VERBATIM
)

include(CTest)
if(BUILD_TESTING)
    add_executable(config_tests
        tests/config_tests.cpp
        src/config/AppConfig.cpp
        src/config/ConfigFile.cpp
        src/physics/Vehicle.cpp
        src/physics/VehicleConfig.cpp
    )
    target_include_directories(config_tests PRIVATE src)
    target_compile_options(config_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME config_tests COMMAND config_tests "${CMAKE_SOURCE_DIR}/config")

    add_executable(game_tests
        tests/game_tests.cpp
        src/config/ConfigFile.cpp
        src/game/LapTimer.cpp
        src/game/RaceSession.cpp
        src/game/Track.cpp
        src/physics/Vehicle.cpp
        src/physics/VehicleConfig.cpp
    )
    target_include_directories(game_tests PRIVATE src)
    target_compile_options(game_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME game_tests COMMAND game_tests "${CMAKE_SOURCE_DIR}/config")
endif()
