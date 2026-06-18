# Lightweight Open-Wheel Sim Prototype

A small native macOS driving prototype built around SDL3, Metal, C++20, and a
fixed 360 Hz simulation loop. The current build provides a procedural test
oval, a more developed IR-18-inspired open-wheel car, material-shaded Metal
rendering, procedural sky, animated/steered wheel presentation, dynamic chase
camera, glass-style HUD, synthesized audio, keyboard driving, timed laps,
runtime tuning menu, personal-best ghost car, editable JSON configuration,
richer four-corner physics telemetry, and a live SDL3 joystick diagnostic
screen.

The retained project brief is in `initial_prompt_instructions.txt`. The
canonical description of the current implementation is
[`PROJECT_STATE.md`](PROJECT_STATE.md).

## Quick Start

Install prerequisites once:

```sh
xcode-select --install
brew install cmake sdl3
```

Then build and launch from the repository root:

```sh
./scripts/build_and_run.sh
```

This is a native app, so there is no local server to start. See
[`docs/running.md`](docs/running.md) for VS Code tasks, manual launch commands,
controls, and troubleshooting.

In VS Code, run `Tasks: Run Task` and choose `Sim: Build and Run`. Full Xcode
is optional and is not required for the normal development workflow.

## Current Status

- Native SDL3 macOS window with a Metal-backed renderer
- Fullscreen toggle, borderless startup option, vsync, and render scale
- Fixed-timestep four-contact-patch vehicle model with Pacejka-lite combined
  slip, banked-track lateral gravity/normal-load adjustment, left/right load
  transfer, current-ride-height aero CoP migration, IMS-stacked gearing, and
  brake bias
- Keyboard controls and a provisional config-driven G29 axis mapping with
  pedal inversion, deadzone, and gamma/sensitivity calibration
- Speed-sensitive keyboard steering assist that slows key steering response and
  clamps road-wheel angle at high speed
- Live SDL joystick/gamepad names, VID/PID, axes, buttons, hats, and haptic flag
- Procedural 2.5-mile test oval with quintic-smoothed banking easements, apron,
  low-lateral-grip/high-drag grass with separate straight-line escape grip,
  plain outer barrier, visible inner infield barrier, catch fencing,
  grandstands, and a darker worn racing surface
- Checkpoint-validated lap timing and invalid-lap detection
- Best-valid-lap ghost car playback for the current run
- Surface grip/resistance changes, speed-sensitive grass-trap speed bleed, and
  off-track dust feedback
- Four-corner bounding-box wall collision against the outer wall and visible
  inner infield wall, plus reset-to-track behavior
- Generated high-detail OBJ IR-18-inspired open-wheel car with correct
  long/low proportions, a very low wedge nose, narrow monocoque, compact
  tapered sidepods, blade-like front wing, compact rear wing, diffuser fins,
  cylindrical wishbones/pushrods, mirrors, halo, helmet/visor, dual exhausts,
  yoke steering wheel, and separate smooth wheel/rim/brake meshes with rounded
  front/rear tire proportions and sidewall detail
- ACES-style filmic tone mapping, gamma correction, richer procedural
  sky/sun/clouds, sky-matched fog, subtle lens vignette/color fringing, contact
  shadows/material occlusion, HDR sharpening, mip-chain bloom, heat shimmer,
  and texture-backed PBR-style material shading for asphalt, grass, concrete,
  pearl paint, carbon, rubber, metal, grandstand/crowd, visor, and brake
  surfaces
- 4x MSAA HDR scene rendering with 4096 shadow maps for cleaner body,
  suspension, wheel, and barrier edges
- Fixed-distance chase camera with constant 50-degree FOV, rearward/upward
  seated cockpit camera with a clearer over-wheel and over-nose sightline,
  direct chase translation to prevent high-speed zoom-out, bank/accel roll
  hints, muted asphalt vibration, and stronger grass/slip/contact shake
- Wheel-mounted 3D digital display for gear, speed, RPM, lap time, and delta
- Glass-style bottom instrument cluster with rounded translucent panels,
  digital speed/gear/RPM readouts, and a curved segmented RPM sweep
- Code-synthesized SDL3 high-rev combustion engine with harmonic V6 firing
  pulses, turbo/mechanical texture, off-throttle burbles, tire-scrub, and
  collision-impact audio
- Escape tuning menu for keyboard steering rates, brake bias, auto-shift, and
  record/ghost reset
- SDL3 haptic force-feedback backend that attempts constant-force streaming,
  then spring/rumble fallbacks, from trail-based front-tire aligning torque
- Self-contained, ad-hoc-signed `.app` bundle with SDL3 and resources

The G29 mapping and force feedback have **not** been verified on physical
hardware. Use the diagnostic screen and FFB status line to identify the axes,
buttons, haptic support, and backend SDL exposes for your wheel, then update
`config/input_default.json` and `config/ffb_default.json`.

## Prerequisites

```sh
xcode-select --install
brew install cmake sdl3
```

If Apple Clang reports that standard headers such as `<string>` cannot be
found, reinstall or repair Xcode Command Line Tools before building.

## Build And Run

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
open build/LightweightSim.app
```

For a terminal launch that keeps log output visible:

```sh
./build/LightweightSim.app/Contents/MacOS/LightweightSim
```

Full Xcode is optional. If it is installed, generate an Xcode project with:

```sh
cmake -S . -B build-xcode -G Xcode
cmake --build build-xcode --config Release
```

Command Line Tools alone are sufficient for VS Code and command-line
development. See [`docs/setup_macos.md`](docs/setup_macos.md).

## Controls

| Action | Control |
| --- | --- |
| Steer | `A`/`D` or left/right arrows |
| Throttle | `W` or up arrow |
| Brake | `S`, down arrow, or Space |
| Reset car | `R` |
| Menu | Escape |
| Menu navigation | Up/down, left/right, Enter |
| Toggle overlay | `F1` |
| Toggle input diagnostics | `F2` |
| Toggle fullscreen | `F11` or Command+F |
| Quit | Command+Q |

## G29 Setup

1. Connect the wheel directly over USB before launching.
2. Open the diagnostic panel with `F2`.
3. Move the wheel and each pedal, then note the changing axis indices.
4. Press both paddles and note the button indices.
5. Update the `wheel` object in `config/input_default.json`.
6. Restart the app after changing config.

The default mapping is only a starting guess. Pedal inversion and axis ordering
can differ across devices and macOS driver paths. The default G29 pedal mapping
assumes inverted SDL axes, where released pedals read near `+1.0` and depressed
pedals read near `-1.0`.

## Architecture

- `src/app`: app lifecycle and fixed-step loop
- `src/audio`: SDL3 stream and synthesized combustion engine/scrub/impact
  audio
- `src/platform`: SDL window lifecycle and resource path lookup
- `src/input`: semantic actions, speed-aware keyboard mapping, and raw SDL diagnostics
- `src/game`: procedural oval, surface queries, race session, barrier containment,
  grass trap behavior, lap timing, and ghost recording/playback
- `src/physics`: config-driven four-contact tire, drivetrain, brake,
  load-transfer, and aero behavior
- `src/render`: native Metal renderer, procedural sky/car/track/fence/
  grandstand meshes, split body/wheel rendering, 4x MSAA HDR scene rendering,
  filmic material shading/fog, camera dynamics, SDF text, and glass HUD
  geometry
- `src/ui`: allocation-free telemetry/device/menu overlay data
- `src/ffb`: SDL3 haptic force-feedback backend with constant-force,
  spring/rumble fallback, and no-op unsupported-device status
- `config`: human-editable graphics, input, vehicle, track, and FFB settings

The hot loops use fixed-size storage and avoid explicit per-frame heap
allocation. Metal/SDL framework internals may still allocate.

More detail is in [`docs/running.md`](docs/running.md),
[`docs/setup_macos.md`](docs/setup_macos.md), [`docs/physics.md`](docs/physics.md),
and [`docs/ffb_macos.md`](docs/ffb_macos.md).
