# Running The App

This is a native macOS application. There is no server to start and nothing
needs to be hosted. Build the `.app`, then launch it locally.

VS Code plus its integrated Terminal is the primary development environment.
Full Xcode is not required.

## Fastest Build And Launch

From the repository root:

```sh
./scripts/build_and_run.sh
```

The script configures a Release build in `build`, builds it, and opens:

```text
build/LightweightSim.app
```

Use a different build directory or build type with environment variables:

```sh
BUILD_DIR=build-debug BUILD_TYPE=Debug ./scripts/build_and_run.sh
```

## Manual Build And Launch

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
open build/LightweightSim.app
```

After the app has already been built, relaunch it with:

```sh
open build/LightweightSim.app
```

To keep SDL/app log output visible in Terminal:

```sh
./build/LightweightSim.app/Contents/MacOS/LightweightSim
```

## Run From VS Code

Open the repository folder in VS Code, then use `Terminal > Run Task`:

- `Sim: Build and Run`: configure a Release build, build it, and open the app
- `Sim: Build Debug`: configure and build the Debug app
- `Sim: Test`: build Debug and run CTest

The default VS Code build task is `Sim: Build Debug`, so Command+Shift+B builds
the app without requiring an Xcode project.

Install the recommended CodeLLDB extension, then use the `Sim: Debug Native App`
launch configuration or press F5 to build and debug the native executable from
VS Code.

## Run Tests

```sh
ctest --test-dir build --output-on-failure
```

## Optional: Run Through Xcode

After installing and selecting full Xcode as described in `setup_macos.md`:

```sh
cmake -S . -B build-xcode -G Xcode \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
open build-xcode/LightweightOpenWheelSim.xcodeproj
```

In Xcode:

1. Select the `LightweightSim` scheme.
2. Select `My Mac` as the run destination.
3. Press the Run button or Command+R.

The generated Xcode project is disposable. Make source/build-setting changes in
the repository and `CMakeLists.txt`, then regenerate the project.

## What You Should See

The app opens a native window showing the procedural 2.5-mile test oval and a
generated high-detail OBJ open-wheel-style car positioned shortly before
start/finish. The car has a sharp flat-panel monocoque, low wedge nose, angular
sidepods with dark radiator intakes, bargeboards, shark fin/airbox, mirrors,
halo, helmet/visor, dual exhausts, carbon floor strakes, livery stripes/number
markings, a downsized animated steering wheel, multi-element wings, winglets,
diffuser fins, cylindrical wishbones and pushrods, separate exposed
smooth-normal wheel/rim/brake meshes with rounded tires and sidewall detail,
visual tire squash, rear brake lamps, clean metallic brake discs, skid-smoke
puffs, and undertray bottoming sparks. Exhaust flame bursts and rear-exhaust
heat shimmer are intentionally removed for a more serious sim presentation.
The cockpit camera sits rearward/upward inside
the monocoque so the driver looks over the wheel and down the nose, and the
steering wheel carries a small 3D digital display for gear, speed, RPM, lap
time, and delta. The front
wheels steer visually and all four wheels roll from speed-derived render
telemetry. The track has a darker worn racing surface with procedural asphalt
aggregate/oil staining, apron/edge lines, grass variation, concrete staining on
walls/apron, a plain white outer barrier, a visible inner infield wall, catch
fencing on both wall lines, grandstands on the straights, a checkered
start/finish marker, and quintic-smoothed banking easements at corner
entry/exit.

World geometry uses ACES-style filmic tone mapping, gamma correction,
procedural material shading, a generated sky with sun glow, richer horizon
color and soft clouds, sky-matched fog, contact-shadow/material-occlusion
grounding under the car, texture-backed normal maps, HDR sharpening,
half-precision two-level bloom, configurable 2x default MSAA edge smoothing,
configurable cached 2048x2048 default local sun shadows, and subtle screen-edge
lens grading.
The chase camera uses a fixed distance, locked height, and constant 50-degree
FOV; `C` toggles to a seated 55-degree chassis-locked cockpit camera with no
cockpit shake.
Chase view keeps asphalt vibration very subtle while increasing shake for
grass, high slip, and wall contact. A glass-style bottom instrument cluster is always drawn
with digital speed/gear/RPM text and a curved RPM sweep. The telemetry/lap HUD
appears in the upper-left, synthesized combustion engine/tire/impact audio
plays through SDL3 when an audio device is available, and the SDL input diagnostic panel is
visible by default. After a valid best lap, a translucent ghost car replays that
lap during the current run. The telemetry overlay also prints the active FFB
status reported by `ForceFeedbackManager`, such as constant force, spring
fallback, rumble fallback, no selected haptic device, or unsupported effects.

Use:

- `W`/`A`/`S`/`D` or arrow keys to drive
- Space to brake
- `R` to reset
- Escape to open/close the tuning menu
- Up/down, left/right, and Enter to use the tuning menu
- `F1` to toggle the full overlay
- `F2` to toggle device diagnostics
- `F11` or Command+F to toggle fullscreen
- Command+Q to quit

## Common Problems

If CMake cannot find SDL3:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
```

If `build/LightweightSim.app` does not exist, the build did not complete.
Review the preceding CMake output before trying to launch it.

If macOS blocks the app, open System Settings > Privacy & Security and approve
the local build. CMake applies an ad-hoc signature for local testing.
