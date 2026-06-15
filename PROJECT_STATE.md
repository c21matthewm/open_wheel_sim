# Project State

Last updated: June 13, 2026

This is the canonical current-state document for the Lightweight Open-Wheel Sim
Prototype. It explains what exists now, how the application works, what has
been verified, and what remains unfinished.

## Maintenance Rule

Update this document in the same change whenever any of these areas change:

- implemented or removed behavior
- architecture or module ownership
- dependencies, build steps, run steps, or packaging
- configuration files or supported controls
- test coverage or completed verification
- known limitations, hardware status, or milestone status

Keep the `Last updated` date accurate. Describe only behavior that currently
exists and clearly label anything that has not been tested on real hardware.

## Current Phase

The project has completed its native macOS foundation, basic-car movement, the
first procedural-oval/lap-timing pass, multiple presentation/gameplay passes,
and the first 3D suspension/drivetrain physics pass. It is a runnable
single-car time trial prototype with a lightweight multi-body IR-18-inspired
open-wheel vehicle model, a four-turn 2.5-mile speedway layout, generated
high-detail OBJ vehicle meshes, texture-backed filmic material shading,
synthesized audio,
SDL3 haptic force-feedback backend plumbing, a generated skybox/sky pass, HDR
post processing, an interactive tuning menu, a personal-best ghost car, fixed
chase/cockpit camera modes, and a more polished HUD/camera presentation with
Cook-Torrance material lighting, material occlusion/contact-shadow grounding,
half-precision two-level bloom,
frosted HDR HUD glass, dynamic suspension rods, 360 Hz default physics timing,
locked-distance chase camera framing, center-protected high-speed post blur,
launch wheelspin-aware automatic shifting, a geometric 9500-RPM-drop speedway
gear stack with shift cooldown, stateful tire temperature/thermal-grip
telemetry, generated high-resolution PBR material maps, a generated skybox used
for environment lighting/reflections, a modern MoTeC-style racing HUD,
configurable 2x default MSAA scene rendering, and a configurable 2048x2048
default sun shadow map.

The current app can:

- launch as a native, ad-hoc-signed macOS `.app`
- create an SDL3 window backed by a native Metal renderer
- render an approximately 2.5-mile four-turn speedway layout with 5/8-mile long
  straights, 1/8-mile short chutes, quarter-mile turns, 256.135 m corner
  radius, config-driven banking with 50 m quintic-smoothstep entry/exit
  easement runouts, inside-asphalt-edge banking pivot, flat apron/infield at
  `Y = 0`, material-tagged asphalt, apron, grass, checkpoint markings, plain
  outer wall/fence, visible inner infield wall/fence beyond a grass runout,
  grandstands, crowd billboard flecks, and texture-backed material detail; the
  asphalt racing groove and edge lines are blended in the shader from a
  track-lateral coordinate rather than drawn as elevated overlay strips
- render and drive a generated high-detail OBJ IR-18-inspired car mesh with
  averaged OBJ vertex normals, approximately 5.1 m length, approximately
  1.9 m width, approximately 1.0 m height, a very low wedged nose, narrow
  monocoque, compact tapered sidepods, recessed cockpit, mirrors, helmet/visor,
  halo, carbon floor/diffuser strakes, modeled suspension joint detail,
  cylindrical wishbones/pushrods, a low blade-like front wing with minimal
  tapered endplates, a compact tensioned rear wing without massive slab
  endplates, dual exhaust pipes, brake lamps, and chase/cockpit cameras
- draw front wheels as separate steered high-detail OBJ wheel meshes and all
  four wheels with physics-derived angular rotation, hub travel, normal-load
  tire compression, rear-size 27-inch-OD/14-inch-width tire geometry, front
  visual scale matching the 25.5-inch-OD/10-inch-width target, rounded tires,
  deep rim lips, hub/spoke/brake geometry, sidewall detail, and shader
  tire-groove/lettering detail
- draw physics-driven chassis heave, pitch, and roll relative to the
  unsprung wheel hubs, plus dynamic Metal wishbone/pushrod rods whose endpoints
  follow chassis pickup points and wheel hubs
- draw rear brake lamps, soft texture-backed smoke particles under high rear
  slip/grass driving, grass/off-track dust coloring, bottoming spark streaks
  when the undertray rides below the aero stall height, session-local
  alpha-blended asphalt skidmarks, and no exhaust flame or rear-exhaust
  heat-shimmer effects
- shade world geometry through an HDR Metal path with texture-backed albedo,
  normal, roughness, and height-style maps, including 4096x4096 asphalt/grass
  maps, 2048x2048 concrete/carbon maps, and a generated lat-long skybox used
  for sky, fog, ambient color, and clearcoat reflections; tangent-space normal
  perturbation; shader-integrated asphalt
  groove/paint lines; ACES-style filmic tone mapping; gamma correction;
  Cook-Torrance GGX/Smith/Schlick PBR-style specular lighting; pearl-metallic
  paint and multi-layer clearcoat response; carbon weave and tire micro-detail;
  colored sun/sky/ground ambient lighting; sky-matched atmospheric fog;
  ground-aware clearcoat sky reflections for paint/carbon/visor glass;
  geometric contact-shadow darkening under the chassis/wheels; material
  occlusion; shader-integrated asphalt aggregate/rubber/oil variation, grass
  blade and mowing variation, concrete staining, and material-specific rubber/
  metal/crowd/brake shading; stronger sun/sky contrast, broadened soft-shadow
  PCF, refined bloom weighting, subtle HDR sharpening, and screen-edge
  vignette/chromatic lens grading; and a skybox-backed generated sky pass with
  sun glow, richer horizon color, cirrus, and procedural drifting clouds
- render a configurable 2048x2048 default sun shadow-map pass on a configurable
  interval that defaults to every other frame, focused by configurable local
  frustum/light-distance settings to roughly an 80 m region around the player
  car, then apply Poisson-disk PCF-filtered dynamic
  shadows in the world shader for the car, wheels, dynamic suspension rods,
  fencing, walls, and grandstand geometry using the cached matching light
  matrix on skipped update frames
- render the scene through configurable 2x default multisampled RGBA16Float HDR
  color and depth targets, resolve the smoothed scene into the single-sample HDR
  texture,
  extract bright highlights through half- and quarter-resolution bloom targets
  using half-precision bloom shader math, and composite the config-weighted
  two-level bloom plus a reduced,
  center-protected speed-weighted radial motion-blur approximation in a final
  post pass before HUD/text rendering
- use a fixed-distance chase camera with constant 50-degree FOV, locked
  distance/height updated directly from the car each frame to avoid high-speed
  follow lag or apparent zoom-out, a world-stable chase height that does not
  inherit sprung-mass heave, configurable bank/acceleration roll hints, no
  clean-asphalt shake by default, startup shake suppression, and configurable
  apron/grass/slip/wall-contact shake
- toggle a cockpit camera with `C` or the configured wheel camera button; the
  cockpit camera is moved backward inside the monocoque and slightly upward
  for a seated over-wheel/over-nose sightline, uses a 55-degree FOV, and
  remains locked to the chassis with no eye smoothing/shake so the cockpit
  frame stays stable
- render a small 3D digital display onto the animated steering wheel using the
  generated SDF font, showing gear, speed in MPH, RPM, lap time, and delta
- drive the procedural car with keyboard input using retuned, smoother
  speed-sensitive keyboard steering rates and a config-driven high-speed
  road-wheel-angle cap that preserves enough steering lock for cornering
- simulate vehicle motion at a fixed default rate of 360 Hz
- render independently from physics and interpolate the vehicle transform
- simulate a sprung chassis with heave, pitch, and roll DOFs, four unsprung
  wheel hubs, spring/damper/anti-roll-bar loads, tire vertical stiffness,
  elastic lateral and longitudinal load transfer, degressive tire load
  sensitivity, static setup camber thrust applied before combined-slip limiting,
  front pneumatic/mechanical trail aligning moment in the tire solver,
  load/speed-dependent transient tire relaxation length,
  Pacejka-lite tire peak/falloff force curves,
  true force-at-corner rigid-body yaw integration from each tire's `r x F`
  moment plus front aligning moment, physical
  per-wheel angular velocity/inertia, wheelspin/lockup slip ratio, combined
  grip/friction-circle limiting, brake bias, IMS-stacked geometric drivetrain
  gearing that drops 12,000 RPM shifts to roughly 9,500 RPM, automatic shift
  cooldown, smooth manual-mode RPM limiter bounce, RPM, automatic upshift
  suppression when 1st/2nd-gear launch wheelspin spikes rear-wheel RPM, strict
  manual mode when automatic transmission is disabled, and per-corner tire
  temperature/thermal-grip evolution from sustained slip/usage
- calculate ground-effect aero from current compressed front/rear ride height
  and rake, including exponential ride-height gain, tunnel-balance CoP shift,
  braking CoP migration, undertray stall/bottoming reduction, asymmetric
  bottoming CoP shift, a small instant current-platform aero-load component,
  and forward CoP migration when the nose rides lower than the rear, alongside
  switchable config-backed Speedway and Road Course aero presets that modify
  downforce, drag, base front aero balance, braking CoP shift, stall height, and
  stall reduction
- apply track banking to physics through lateral gravity decomposed relative to
  the track tangent (scaling with heading error) and bank-adjusted normal-load
  calculations, preventing sustained lateral slip on straights
- apply asphalt/apron/grass lateral grip, grass-specific longitudinal grip,
  and rolling-resistance multipliers to the tire-force/resistance model; the
  tire solver uses separate longitudinal and lateral friction limits so the
  default grass still slides easily while having enough straight-line bite for
  keyboard throttle escape
- apply extra grass-trap rolling resistance, speed-sensitive damping, and
  warmer off-track dust puffs when the car leaves the racing surface
- contain the vehicle at the outer wall and visible inner infield wall with a
  yaw-aware four-corner bounding-box collision pass that applies contact-point
  linear and yaw impulse response, then reset it near start/finish when
  requested
- time laps using ordered checkpoints, invalidate off-asphalt laps, and record
  the best valid completed lap as an in-memory translucent ghost car
- show vehicle, performance, input, and raw SDL device diagnostics
- draw lightweight HUD backing panels, darker worn racing surface, generated
  material texture detail, shader-integrated asphalt rubber/paint markings,
  outside catch fencing, grandstands, and a checkered start/finish marker with
  a compact generated WebP texture set for material realism
- draw a glass-styled MoTeC-like bottom instrument cluster with rounded
  translucent panels, glow accents, prominent digital MPH/gear/RPM text,
  brake/throttle bars, tire temperature/usage widgets, dynamic LED shift
  lights, a curved segmented RPM sweep, generated SDF text for sharper
  HUD/debug rendering, and a HUD shader that samples the resolved HDR scene
  behind the panels with a compact 7-tap blur for a frosted refractive glass
  effect
- synthesize lightweight SDL3 audio at runtime with a high-revving procedural
  combustion engine layer that combines V6 firing-rate pulse shaping, multiple
  harmonic orders, smoothed RPM/throttle response, turbo whistle/hiss,
  mechanical noise, soft overdrive, off-throttle burbles/pops, tire
  scrub/squeal noise, and collision impact thuds without external audio assets
- show an Escape menu for runtime keyboard steering rate tuning, brake-bias
  adjustment, automatic-transmission toggling, aero preset toggling, and
  record/ghost reset
- enumerate connected SDL joysticks/gamepads and show axes/buttons/hats
- map configured wheel axes/buttons with deadzones, G29-style pedal inversion,
  steering/throttle/brake gamma curves, a configurable wheel steering
  sensitivity multiplier, paddle shifting, and a provisional wheel
  camera-toggle button
- attempt SDL3 haptic force feedback from the selected wheel with constant-force
  streaming, spring fallback, rumble fallback, and clear no-op status when
  unsupported
- derive FFB force from front tire lateral forces multiplied by pneumatic and
  mechanical trail, pneumatic-trail/understeer lightening past peak slip,
  low-speed centering, steering damping, grass/apron vibration, and wall-contact
  jolts
- load graphics, input, vehicle body/suspension/tire/aero/drivetrain,
  oval-track, and FFB settings from JSON
- optionally run a timed live render benchmark with
  `SIM_BENCHMARK_SECONDS=<seconds>`; the app prints FPS, frame time, physics
  time, render time, and physics steps/sec before exiting
- toggle fullscreen and diagnostic overlays
- package SDL3, configs, assets, and documentation inside the app bundle

## Important Limitations

- The current physics now has sprung heave/pitch/roll, four unsprung wheel
  hubs, spring/damper/ARB loads, tire relaxation length, per-wheel angular
  velocity, and ride-height-sensitive aero. It is still a purpose-built
  lightweight simulator model rather than a full finite-element tire model,
  rigid-body collision engine, or licensed commercial-grade vehicle dynamics
  solver.
- Wheel hub vertical dynamics assume a smooth local road plane from the sampled
  track pose. There are no per-wheel terrain height samples, potholes, curbs,
  or arbitrary 3D mesh contacts yet, so suspension travel on banking is a
  smooth approximation rather than true independent track-surface probing.
- Brake discs are intentionally rendered as clean metallic parts without the
  previous red heat-glow presentation. There is still no physical brake thermal
  simulation.
- Tire smoke/dust and undertray sparks are renderer-side presentation effects
  derived from rear tire usage/slip, surface type, and floor-strike telemetry;
  they are not fluid, debris, or material-removal simulations. Exhaust flame
  particles and rear-exhaust heat shimmer have been removed for a more serious
  sim presentation.
- The material texture set is generated and compact, not scanned/photogrammetry
  source art. It now includes 4K asphalt/grass maps, 2K concrete/carbon maps,
  roughness/height-style maps, and a skybox, but it is still an approximate
  lightweight visual layer rather than licensed track-scan source art.
- The OBJ car, wheel, and steering-wheel meshes are generated project assets,
  not licensed third-party models. They now follow the retained IR-18
  description more closely: low wedged nose, narrow monocoque, separated open
  wheels, compact sidepods, low blade front wing, compact rear wing, halo,
  helmet/visor, diffuser, suspension rods, 27-inch rear tire mesh, scaled
  25.5-inch front tires, and a yoke steering wheel. They are still generated
  meshes rather than production scanned or licensed commercial models.
- The four-turn 2.5-mile layout uses real-world-style dimensions, but the
  in-game/config/documentation track name remains generic to avoid using a
  trademarked venue name as a product asset.
- The asphalt groove/edge-line shader constants are tuned for the current oval
  dimensions; if road/apron/wall widths change substantially, the visual line
  placement should be retuned or moved into config/uniforms.
- Skidmarks persist only for the current renderer session and stop appending
  after the fixed skidmark vertex buffer fills. They are visual tire marks, not
  tire-rubber accumulation affecting physics.
- Shadow mapping is implemented as a single sun-focused orthographic shadow map
  centered near the player car. It is intended for local car/fence/grandstand
  grounding, not full-track long-distance shadow coverage. The default
  2048x2048 map uses a tightened roughly 80 m local frustum, broadened
  Poisson-disk filtering, and a default every-other-frame update cadence with a
  cached light matrix; target TV/display verification is still useful.
- Bloom now uses a lightweight two-level downsample chain with half- and
  quarter-resolution targets and half-precision bloom shader math, but it is
  still simpler than a large-radius production bloom pyramid. Motion blur
  remains a screen-space radial approximation rather than a true per-pixel
  velocity-buffer pass.
- HUD glass samples and blurs the resolved HDR scene buffer behind the UI
  panels, but the blur is a compact 7-tap shader pass with configurable
  blur/refraction radii rather than a large-radius production separable blur or
  full optical glass model.
- The current tire model uses relaxation length for lateral slip, dynamic slip
  ratio for longitudinal force, load/speed-dependent relaxation length, a
  compact Pacejka-lite peak/falloff curve, degressive load sensitivity, static
  setup camber thrust, friction-circle combined-slip limiting, front
  pneumatic/mechanical trail aligning moment, and a simple slip/usage-driven
  tire temperature/thermal-grip state. It still has no pressure, wear, carcass
  modes, dynamic camber gain, rear pneumatic trail, contact-patch deformation,
  or fully validated tire test data.
- Wheel angular velocity supports wheelspin and lockup behavior. Engine RPM is
  coupled to driven wheel speed with an idle clamp for launchability; a full
  clutch, anti-stall, starter, and engine-stall simulation is not implemented.
- Ground-effect downforce reacts to front/rear ride height and rake with an
  undertray stall floor and config-backed aero package presets, but it is an
  analytic model, not CFD or wind-tunnel-derived aero-map data.
- Oval wall containment uses a yaw-aware four-corner vehicle bounding box
  against the analytic outer and inner wall offsets. It prevents center-only
  wall clipping and adds contact-point yaw impulse response, but there is still
  no general arbitrary-geometry collision system.
- Runtime menu adjustments are not persisted back to JSON yet; restart returns
  to the config file defaults.
- The ghost car is an in-memory personal-best lap for the current run only; it
  is not saved to disk between launches.
- Audio is synthesized and intentionally lightweight. The engine layer now aims
  for a high-revving combustion Indycar character with pulse harmonics,
  mechanical noise, overdrive, turbo hiss, and off-throttle pops, but it is not
  a physically accurate engine, exhaust, turbo, tire, or collision sound model.
- G29 steering, pedals, paddles, and FFB have provisional config/code paths but
  have not been verified with a physical G29 on macOS. The current wheel
  steering path applies a `steer_sensitivity` multiplier and avoids smoothing,
  but the final value should still be tuned against real hardware.
- The SDL3 haptic backend is implemented, but actual G29 force-feedback support
  depends on whether macOS/SDL exposes constant force, spring, or rumble for the
  connected wheel. Unsupported paths fall back or report no-op status.
- There is no live config reload, persistent calibration saving, telemetry CSV
  output, or full menu system beyond the current Escape tuning menu.
- Full Xcode project generation has not been verified and is optional. VS Code
  and command-line builds are the primary workflow.

## How The Application Works

### Startup

1. `main.cpp` creates `App` and calls `App::run`.
2. `App` locates and loads JSON config files from the working tree or app
   bundle resources.
3. `WindowSDL` initializes SDL3 and creates a Metal-capable macOS window.
4. `InputManager` opens available SDL joystick devices.
5. `ForceFeedbackManager` loads SDL haptic settings and waits for the selected
   wheel.
6. `Vehicle` receives its config-driven body, suspension, tire, aero, brake,
   and drivetrain parameters.
7. `RaceSession` creates the four-turn speedway layout and resets the vehicle
   shortly before start/finish.
8. `MetalRenderer` creates the SDL Metal view, Metal pipelines, oval geometry,
   dynamic suspension/FX buffers, MSAA HDR/depth targets, resolved
   HDR/bloom/shadow targets, generated high-detail OBJ car/wheel/steering
   meshes, high-resolution generated mipmapped PBR material maps, skybox,
   smoke texture, and generated SDF font atlas.

### Per-Frame Flow

1. SDL events are polled and converted into semantic `InputActions`.
2. Raw joystick state is sampled for mapping and the diagnostic display, while
   keyboard steering rate is scaled down as vehicle speed rises.
3. The Escape menu can pause physics stepping while runtime tuning values are
   adjusted.
4. `GameLoop` accumulates elapsed wall-clock time.
5. `RaceSession` samples the current surface/banking and advances
   `Vehicle::step` zero or more times at the configured physics rate.
6. The race session resolves the outer/inner barriers, applies extra grass
   damping, advances checkpoint lap timing, and updates the in-memory ghost
   recorder/playback state.
7. `SoundManager` updates its synthesized engine, tire, and impact parameters
   from vehicle/input/race telemetry.
8. `ForceFeedbackManager` computes and streams physics-derived normalized FFB
   through the active SDL haptic backend when available.
9. The renderer interpolates between the previous and current vehicle states.
10. The renderer updates the selected camera mode, chassis pitch/roll/heave,
    wheel hub travel/rotation/deformation, dynamic suspension rods, sky time,
    bottoming sparks, and session-local skidmark geometry when rear tire slip
    is high.
11. Metal updates the sun shadow depth pass for world/player geometry on the
    configured interval and reuses the cached shadow map/matrix on skipped
    frames.
12. Metal renders the skybox-backed sky, oval, player vehicle body, dynamic
    suspension rods, unsprung wheels, ghost vehicle if available, skidmarks,
    and soft smoke/dust/spark particles into configurable 2x default multisampled
    RGBA16Float HDR/depth targets, then resolves color into the single-sample
    HDR texture.
13. Metal extracts bright pixels through half- and quarter-resolution bloom
    targets using half-precision bloom shader arithmetic.
14. Metal composites HDR color, two-level bloom, speed-weighted radial blur,
    ACES tone mapping, gamma correction, and lens grading into the drawable.
15. Metal draws the HDR-sampling frosted-glass HUD, menu text, and
    debug/device/lap overlay onto the final drawable.

Physics behavior is not tied to the render frame rate.

## Main Modules

- `src/app`: application lifecycle and fixed-timestep coordination
- `src/audio`: SDL3 audio-stream ownership and code-synthesized high-rev
  combustion engine, turbo/mechanical noise, off-throttle burbles, tire scrub,
  and collision sounds
- `src/platform`: SDL window ownership and resource-path lookup
- `src/config`: small dependency-free JSON config reader and typed app config
- `src/input`: semantic controls, keyboard input, SDL device enumeration, and
  provisional wheel mapping, including speed-sensitive keyboard steering rate,
  pedal inversion, deadzones, gamma curves, direct wheel steering sensitivity,
  and the configured camera-toggle button
- `src/game`: shared four-turn speedway geometry math, surface queries with
  split lateral/longitudinal grip telemetry, race coordination, quintic
  banking-easement runouts, yaw-aware four-corner inner/outer wall containment,
  grass-trap rolling resistance and speed-sensitive speed bleed, checkpoint lap
  timing, and in-memory ghost-lap recording/playback
- `src/physics`: config-driven sprung/unsprung vehicle state, four-corner
  spring/damper/ARB suspension loads, tire vertical stiffness,
  load/speed-dependent transient tire relaxation, Pacejka-lite tire forces with
  peak/falloff behavior, degressive load-sensitive friction limits, static mirrored camber thrust, solver-owned
  front pneumatic/mechanical trail telemetry and yaw aligning moment,
  combined-slip tire forces with separate longitudinal/lateral friction limits,
  force-at-corner rigid-body tire torque integration, dynamic tire
  temperature/thermal-grip state, dynamic wheel angular
  velocity/slip ratio, banking, IMS speedway geometric gearing with shift
  cooldown, strict manual/automatic transmission modes, smooth manual-mode RPM
  limiter bounce, drivetrain, brake, current-ride-height-sensitive ground-effect aero with
  dynamic CoP migration and config-backed speedway/road-course package presets,
  load-transfer, high-speed road-wheel-angle capping,
  contact-offset wall impulse response, launch wheelspin-aware automatic
  upshift gating, and telemetry behavior
- `src/render`: Metal renderer, compact ImageIO/CoreGraphics texture loading,
  header-only OBJ loading, generated high-detail OBJ car/wheel/steering meshes,
  generated mipmapped WebP PBR material maps, generated skybox/smoke maps,
  procedural track/fence/grandstand/HUD geometry, split car-body/wheel
  rendering, configurable 2x default MSAA HDR scene rendering,
  material-tagged HDR filmic
  Cook-Torrance lighting, track-lateral asphalt marking shader, tangent-space
  normal mapping, roughness/height map sampling, pearl-metallic clearcoat,
  carbon/tire micro-detail, skybox-backed clearcoat environment reflection,
  material occlusion and contact-shadow grounding, visor-glass shading,
  configurable 2048-default Poisson-filtered shadow-map pass,
  half-precision half/quarter bloom chain, HDR
  sharpening, livery masks, sky-matched fog, fixed
  chase/cockpit camera modes that consume chassis attitude, direct
  locked-distance chase camera translation, rearward/upward seated cockpit
  sightline, center-protected speed blur, dynamic
  suspension rod rendering, SDF font atlas, 3D steering-wheel display text,
  HDR-sampling frosted MoTeC-style HUD geometry with shift LEDs and tire
  widgets, soft smoke/dust/sparks, persistent visual skidmarks, and shaders
- `src/ui`: fixed-capacity HUD, debug, physics, lap, and device-diagnostic
  overlay plus current Escape tuning-menu text
- `src/ffb`: SDL3 haptic force-feedback backend with constant-force streaming,
  spring/rumble fallbacks, front tire pneumatic/mechanical-trail aligning
  torque, peak-slip lightening, smoothing, clipping, and unsupported-device
  no-op status
- `config`: editable graphics, input, vehicle, four-turn oval track, and FFB
  data
- `assets/meshes`: generated Wavefront OBJ meshes for the car body, wheels, and
  animated steering wheel, including an IR-18-inspired low wedge nose, tight
  monocoque, compact tapered sidepods, blade-like front wing, compact rear
  wing, diffuser/floor strakes, suspension rods/joints, halo, helmet/visor,
  rear-size tire/rim/brake geometry with front visual scaling, and yoke wheel
  detail
- `assets/textures`: generated WebP PBR texture set with 4096x4096
  asphalt/grass albedo, normal, roughness, and height maps; 2048x2048
  concrete/carbon albedo/normal/roughness maps; a generated lat-long skybox;
  and a 256x256 soft smoke puff. These are mipmapped at startup and
  intentionally compressed
- `docs`: setup, running, architecture, physics, tuning, FFB, and packaging

`CMakeLists.txt` is the source of truth for project files and build settings.
Generated Xcode projects should be regenerated from CMake rather than edited as
the authoritative project definition. CMake is configured to generate an Xcode
scheme for `LightweightSim` when the Xcode generator is available.

## Configuration

- `config/graphics_default.json`: window, fullscreen, vsync, render scale,
  MSAA sample count, shadow map size, shadow update interval, shadow local
  frustum/light placement, bloom blend weights, HUD glass blur/refraction
  radii, and physics timing; the default physics tick is 360 Hz for the stiff
  IR-18-style spring/unsprung-mass setup, the default MSAA sample count is 2,
  the default shadow map size is 2048, and the default shadow update interval
  is 2 frames
- `config/input_default.json`: keyboard response, provisional wheel mapping,
  camera-toggle button, pedal inversion, deadzones, wheel steering sensitivity,
  and wheel/pedal gamma curves; the default keyboard steering rates are
  `steer_rate = 3.2` and `return_rate = 5.2`
- `config/vehicle_openwheel_default.json`: vehicle body/inertia, steering,
  suspension, four-corner tire/load-transfer, drivetrain/wheel-inertia, brake,
  IMS-stacked geometric speedway gearing, automatic transmission toggle,
  Pacejka-lite tire curve shape, degressive tire load-sensitivity coefficients,
  speedway and road-course static camber setup values, tire pneumatic and
  mechanical trail values, load/speed-dependent tire relaxation length values,
  speedway and road-course aero preset packages,
  ground-effect aero/CoP sensitivity, resistance parameters, and high-speed
  steering cap values; the default high-speed steering scale is `0.22`, the
  default 6th gear redlines at about 239 mph with the configured tire radius,
  and each 12,000 RPM upshift drops to about 9,500 RPM
- `config/ffb_default.json`: SDL haptic enable flag and physics FFB gains for
  master strength, centering, damping, pneumatic/mechanical trail aligning
  torque, understeer lightening, grass vibration, collision jolts, and max force
- `config/track_oval_default.json`: four-turn oval dimensions, short-chute
  length, banking, banking-easement runout length, outer/inner wall offsets,
  lateral surface grip, grass longitudinal escape grip, resistance multipliers,
  wall response, spawn position, and render resolution

Config changes require an app restart. Live reload is not implemented.
Escape-menu steering-rate/brake-bias/auto-shift changes are runtime-only.

## Build And Run State

Required command-line dependencies:

- Apple Clang and macOS SDK from Command Line Tools or full Xcode
- CMake
- SDL3
- Apple CoreGraphics and ImageIO frameworks from the macOS SDK for decoding the
  compact WebP material textures

The primary development workflow is VS Code with its integrated Terminal and
the build/run/test tasks in `.vscode/tasks.json`. `.vscode/launch.json`
provides a native CodeLLDB debug launch. Full Xcode is not required.

The quickest normal workflow from the repository root is:

```sh
./scripts/build_and_run.sh
```

Manual equivalent:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
open build/LightweightSim.app
```

Timed render benchmark:

```sh
SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim
```

See `docs/running.md` for launch options and `docs/setup_macos.md` for
toolchain troubleshooting and optional Xcode project generation.

## Latest Verification

Verified on June 13, 2026 after the 360 Hz physics timing update, keyboard
steering retune, IR-18 mesh generator rewrite, regenerated OBJ assets, visual
wheelbase/tire-scale alignment, cockpit camera update, locked-distance chase
camera fix, center-protected speed-blur reduction, launch wheelspin upshift
gate, high-rev combustion audio synthesis update, four-corner wall collision,
visible inner wall/fence, split grass longitudinal/lateral traction, speed-
sensitive grass-trap damping, rearward/upward cockpit camera repositioning,
lighting/material/post-processing overhaul, IMS speedway gearing,
Pacejka-lite tire force curves, current-platform aero CoP response, trail-based
FFB aligning torque, geometric 9500-RPM-drop gear stack, automatic shift
cooldown, strict `automatic_transmission` config/menu control, smooth
manual-mode RPM limiter bounce, force-at-corner rigid-body yaw integration,
tire temperature/thermal-grip telemetry, generated PBR texture maps,
skybox-backed environment lighting/reflections, removed exhaust flame/heat
shimmer effects, the MoTeC-style HUD pass, the R-1 render-performance
shadow-map recovery pass, the R-2 MSAA recovery pass, the R-3 bloom-chain
simplification pass, the R-4 HUD glass tap-count reduction, the R-5
shadow-update interval pass, the R-6 half-precision bloom shader pass, the
P-1 degressive tire load-sensitivity pass, the P-2 static camber-thrust pass,
the P-3 pneumatic-trail aligning-moment pass, the P-4 load-dependent
relaxation-length pass, the P-5 config-backed aero package pass, and the render
config-tuning compliance pass:

- `python3 scripts/generate_geometry.py` regenerated `assets/meshes/car.obj`,
  `assets/meshes/wheel.obj`, and `assets/meshes/steering_wheel.obj`
- the generator reported:
  `car.obj` 4,592 vertices with bounds `X -0.984..0.984`,
  `Y 0.055..0.985`, `Z -2.532..2.562`;
  `wheel.obj` 7,968 vertices with bounds `X -0.188..0.188`,
  `Y -0.345..0.345`, `Z -0.345..0.345`;
  `steering_wheel.obj` 3,664 vertices with bounds `X -0.201..0.201`,
  `Y -0.145..0.143`, `Z -0.028..0.024`
- `python3 scripts/generate_pbr_textures.py` regenerated the WebP material
  texture set, including 4096x4096 asphalt/grass albedo, normal, roughness, and
  height maps; 2048x2048 concrete/carbon maps; and
  `assets/textures/skybox_ims_evening.webp`
- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure -C Release` passed
  `config_tests` and `game_tests`; `config_tests` now includes the 360 Hz
  physics-rate check, IMS speedway 6th-gear redline, geometric 9500 RPM
  post-shift drop checks, Pacejka-lite tire/aero config load checks,
  degressive load-sensitivity config and high-load cornering regression checks,
  camber config, straight-line camber-cancellation, and camber-thrust regression checks,
  pneumatic-trail telemetry and aligning-yaw-moment regression checks,
  load-dependent relaxation-length config and telemetry regression checks,
  speedway/road-course aero package config and behavior regression checks,
  dynamic tire temperature/thermal-grip telemetry checks, a rigid-body steering/yaw
  regression check, manual shift-cooldown and manual limiter regression checks,
  and a full-throttle launch regression check that prevents automatic shifting
  past 2nd while the car is below the 2nd-gear launch threshold. `game_tests`
  verifies the inner wall
  sits beyond a grass runout, that inner/outer wall corner penetrations are
  caught before the vehicle center crosses the wall, and that grass keeps low
  lateral grip while providing enough straight-line traction for keyboard
  throttle escape
- after the R-1 configurable 2048x2048 shadow-map and tightened local-frustum
  change, `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched the live Metal app and exited automatically with:
  `FPS 43.0`, `FRAME 23.28 ms`, `PHYS 0.037 ms`, `RENDER 22.68 ms`,
  and `PHYS_STEPS 359.0/s`. This was essentially flat versus the pre-R-1
  benchmark (`FPS 42.6`, `RENDER 22.62 ms`), indicating that the shadow pass
  is probably not the dominant render cost on this machine without further
  Metal Frame Debugger confirmation.
- after the R-2 configurable 2x MSAA and memoryless MSAA target change,
  `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  exited with: `FPS 48.1`, `FRAME 20.79 ms`, `PHYS 0.036 ms`,
  `RENDER 20.17 ms`, and `PHYS_STEPS 357.8/s`.
- after the R-3 removal of the eighth-resolution bloom pass, the same benchmark
  exited with: `FPS 47.9`, `FRAME 20.89 ms`, `PHYS 0.034 ms`,
  `RENDER 20.46 ms`, and `PHYS_STEPS 359.1/s`, indicating no measurable gain
  from that pass removal on this run.
- after the R-4 HUD glass blur reduction from 25 blur taps to 7 blur taps,
  repeated benchmark samples exited with `FPS 45.9-46.5`, `FRAME 21.79-21.51 ms`,
  `PHYS 0.036-0.037 ms`, `RENDER 21.26-21.02 ms`, and
  `PHYS_STEPS 358.4-358.5/s`, indicating the HUD tap reduction did not produce
  a measurable frame-time win in this run.
- after the R-5 cached shadow-map update interval defaulting to every other
  frame, the benchmark exited with: `FPS 48.2`, `FRAME 20.73 ms`,
  `PHYS 0.035 ms`, `RENDER 20.27 ms`, and `PHYS_STEPS 358.5/s`.
- after the R-6 half-precision bloom shader change, the benchmark exited with:
  `FPS 49.5`, `FRAME 20.21 ms`, `PHYS 0.034 ms`, `RENDER 19.78 ms`, and
  `PHYS_STEPS 358.4/s`.
- before the render config-tuning compliance pass, the current renderer
  benchmarked at `FPS 46.7`, `FRAME 21.41 ms`, `PHYS 0.036 ms`,
  `RENDER 20.58 ms`, and `PHYS_STEPS 359.2/s`. After moving the remaining
  shadow-frustum, bloom-weight, and HUD-glass tuning constants into
  `graphics_default.json`, the same benchmark exited with `FPS 46.9`,
  `FRAME 21.32 ms`, `PHYS 0.042 ms`, `RENDER 20.82 ms`, and
  `PHYS_STEPS 359.2/s`.
- final verification after the P-1 through P-5 physics upgrade rebuilt cleanly,
  passed `ctest`, and benchmarked at `FPS 43.0`, `FRAME 23.27 ms`,
  `PHYS 0.038 ms`, `RENDER 22.41 ms`, and `PHYS_STEPS 358.4/s`. The fixed
  360 Hz physics loop remained stable and inexpensive; this run did not reach a
  stable 60 FPS render cadence on this machine.
- the self-contained Release app was approximately 11 MB, under the current
  100 MB app/asset budget
- the full asset folder was approximately 8.3 MB; generated OBJ meshes were
  approximately 1.1 MB and generated WebP textures were approximately 7.2 MB

Not verified:

- standalone Metal shader precompilation from the shell; the installed Command
  Line Tools do not include the `metal` compiler utility, so shader compilation
  is still expected to occur at app startup through Metal's runtime library
  creation path
- Xcode Instruments / Metal System Trace profiling from this shell; `xcrun`
  could not find the `xctrace` utility, so render-pass attribution still needs
  GUI Instruments or a full Xcode command-line tools installation
- human visual sign-off for the four-turn track layout, regenerated IR-18-style
  car/wheel/steering meshes, chassis pitch/roll, animated suspension rods,
  wheel hub travel, revised seated cockpit/chase camera feel, wheel-mounted
  text display, high-resolution mipmapped PBR textures, skybox reflections,
  Cook-Torrance highlights, material occlusion/contact-shadow grounding,
  ground-aware clearcoat reflection, revised sky/material/post grade, retuned
  half-precision two-level bloom, 2x default MSAA edges, 2048 local soft shadows,
  MoTeC HUD frosted glass,
  removed exhaust flame/heat shimmer effects, soft smoke/dust/sparks, and
  skidmark path on a physical display
- human-driven confirmation that the corner-entry/exit banking hump is gone
- human listening/tuning pass for the upgraded procedural combustion engine
  timbre, off-throttle burbles, tire scrub balance, and impact levels on target
  speakers/headphones
- physical G29 input mapping, including the direct steering sensitivity and
  configured wheel camera button
- any G29/macOS force-feedback path
- stable 60 Hz-class render cadence on the final target TV/display hardware
  with its selected resolution, refresh mode, and fullscreen/display settings
- complete timed laps driven by a human on target hardware

## Future Milestones: Modular Track System

The current `Track` class already owns the four-turn speedway pose/sample
math, but multiple track support should split this into a polymorphic track
interface and concrete track implementations.

Proposed base interface:

```cpp
class Track {
public:
    virtual ~Track() = default;
    virtual TrackPose poseAtDistance(float distanceM) const = 0;
    virtual TrackSample sample(float positionX, float positionZ) const = 0;
    virtual TrackPose spawnPose() const = 0;
    virtual float lapLengthM() const = 0;
    virtual CheckpointState checkpointState(float distanceM) const = 0;
    virtual BarrierSample outerBarrier(float positionX, float positionZ) const = 0;
    virtual BarrierSample innerBarrier(float positionX, float positionZ) const = 0;
};
```

Milestone architecture:

- Rename the current concrete oval implementation to something like
  `OvalTrack` and keep the existing config as `track_oval_default.json`.
- Move shared `TrackPose`, `TrackSample`, `TrackSurface`, checkpoint, spawn,
  and barrier-collision data structures into a common track header.
- Let `RaceSession` own a `std::unique_ptr<Track>` or equivalent factory
  result selected from track config, so lap timing, spawn selection, surface
  multipliers, and barrier containment are track-agnostic.
- Replace the current inner/outer offset containment with signed barrier fields
  returned by each track implementation. The oval can keep analytic fields;
  future road courses can use piecewise centerline segments, splines, or
  sampled collision strips.
- Add a second natural-terrain road course of roughly 2.2 miles and 11 turns,
  inspired by Laguna Seca or Road America without using licensed venue assets.
  It should include elevation change, curb/grass transitions, slower corners,
  and a long straight to exercise pitch, roll, heave, tire relaxation,
  wheelspin/lockup, and ride-height-sensitive downforce outside the current
  smooth speedway use case.
- Extend renderer track generation so each concrete track can emit its own
  road mesh, grass/infield/outfield mesh, barriers, fencing, checkpoints, and
  spawn visuals while sharing material/shadow/HDR pipelines.

## Recommended Next Work

1. Connect a physical G29 and record the macOS SDL axis/button layout.
2. Verify the overlay-reported SDL haptic backend on a real G29: constant
   force, spring fallback, rumble fallback, or unsupported/no-op.
3. Persist runtime menu/calibration changes back to user config.
4. Tune the sprung/unsprung tire, aero, and drivetrain model through real
   driving laps and telemetry.
5. Add collision telemetry and better reset points.
6. Add optional telemetry CSV logging for tuning comparisons.
7. Add persistent ghost/best-lap save data if the in-memory ghost proves useful.
8. Tune FFB sign/gain/smoothing on real hardware after confirming the macOS
   haptic path.
