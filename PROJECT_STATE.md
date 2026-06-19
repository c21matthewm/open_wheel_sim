# Project State

Last updated: June 19, 2026

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
open-wheel vehicle model, a four-turn 2.5-mile speedway layout, a procedural
3.6 km natural-terrain "Hill Circuit" road course, generated
high-detail OBJ vehicle meshes, texture-backed filmic material shading,
synthesized audio,
SDL3 haptic force-feedback backend plumbing, a generated skybox/sky pass, HDR
post processing, a modern Home/Racing app flow with pre-race track/setup
selection, an interactive tuning menu, a personal-best ghost car, fixed
chase/cockpit camera modes, and a more polished HUD/camera presentation with
Cook-Torrance material lighting, material occlusion/contact-shadow grounding,
half-precision two-level bloom,
frosted HDR HUD glass, dynamic suspension rods, 360 Hz default physics timing,
locked-distance chase camera framing, center-protected high-speed post blur,
launch wheelspin-aware automatic shifting, a geometric 9500-RPM-drop speedway
gear stack with shift cooldown, stateful tire temperature/thermal-grip
telemetry, tire wear, fuel consumption with Lean/Standard/Rich engine maps,
runtime front/rear wing and brake-bias setup controls, bi-linear
bump/rebound damping, per-wheel surface/contact-height sampling, generated
compact PBR material maps, a generated skybox used for environment lighting/reflections,
a modern MoTeC-style racing HUD with wheelspin/slip feedback, configurable 2x
default MSAA scene rendering, ring-buffered 2048-vertex visual skidmarks, and
a configurable 2048x2048 default sun shadow map.

The current app can:

- boot to a sleek Home screen instead of immediately dropping onto the track,
  keep the 360 Hz physics loop idle while Home is active, select Oval or Hill
  Circuit before starting, adjust front wing, rear wing, and brake bias before
  entering the session, and return from the ESC menu to Home
- launch as a native, ad-hoc-signed macOS `.app`
- create an SDL3 window backed by a native Metal renderer
- render an approximately 2.5-mile four-turn speedway layout with 5/8-mile long
  straights, 1/8-mile short chutes, quarter-mile turns, 256.135 m corner
  radius, the start/finish Yard of Bricks located 304.8 meters past the center of the front straightaway, config-driven banking with 50 m quintic-smoothstep entry/exit
  easement runouts, inside-asphalt-edge banking pivot, flat apron/infield at
  `Y = 0`, material-tagged asphalt, apron, grass, checkpoint markings, plain
  outer wall/fence, visible inner infield wall/fence beyond a grass runout,
  grandstands, crowd billboard flecks, and texture-backed material detail; the
  asphalt racing groove and edge lines are blended in the shader from a
  track-lateral coordinate rather than drawn as elevated overlay strips
- render and drive the selectable "Hill Circuit" road course, a procedural
  3.6 km counterclockwise natural-terrain circuit with 11 turns, spline-driven
  elevation, curb strips, grass runoff, armco-style barriers, selected catch
  fencing, two grandstand zones, and an earthen corkscrew viewing berm, all
  using the existing material/shadow/HDR pipeline
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
  normal, roughness, and height-style maps, including 2048x2048 asphalt/grass
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
  car; snap the light center to the map's world-space texel grid; submit only
  static ground ranges whose 3D bounds intersect the actual light frustum; then
  apply Poisson-disk PCF-filtered dynamic
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
  speed-sensitive keyboard steering rates and a speed-sensitive keyboard target
  limit that prevents held digital steering from commanding full lock at
  200 mph; direct wheel input still maps to the configured physical rack range
- simulate vehicle motion at a fixed default rate of 360 Hz
- render independently from physics and interpolate the vehicle transform
- simulate a sprung chassis with heave, pitch, and roll DOFs, four unsprung
  wheel hubs, spring/damper/anti-roll-bar loads, tire vertical stiffness,
  separate bump/rebound damping per axle,
  elastic lateral load transfer and config-filtered longitudinal load transfer,
  degressive tire load
  sensitivity, suspension-coupled dynamic camber gain and camber thrust applied
  before combined-slip limiting,
  front and rear pneumatic/mechanical trail aligning moment in the tire solver,
  load/speed-dependent transient tire relaxation length,
  Pacejka-lite tire peak/falloff force curves with config-backed minimum
  peak-capable stiffness-factor guarding,
  longitudinal slip-ratio relaxation for progressive wheelspin/lockup buildup,
  induced tire scrub drag from lateral slip, speed-dependent cornering
  stiffness reduction from wheel rotational speed,
  true force-at-corner rigid-body yaw integration from each tire's `r x F`
  moment plus solver-owned tire aligning moments, physical
  per-wheel angular velocity/inertia, wheelspin/lockup slip ratio, asymmetric
  combined-slip friction-ellipse limiting, brake bias, config-driven fixed-size
  piecewise-linear engine torque curve with near-redline limiter margins,
  rear clutch-pack limited-slip differential preload/ramp locking, low-drag
  speedway aero, IMS-stacked geometric drivetrain
  gearing that drops 12,000 RPM shifts to roughly 9,500 RPM, automatic shift
  cooldown, smooth manual-mode RPM limiter bounce, RPM, automatic upshift
  suppression when 1st/2nd/3rd-gear launch wheelspin spikes rear-wheel RPM, strict
  manual mode when automatic transmission is disabled, and per-corner tire
  temperature evolution from sustained slip/usage with a Gaussian optimal
  thermal-grip window and cold/overheated grip loss
- derive front/rear roll-axis load-transfer distribution from setup spring
  rates and anti-roll-bar Nm/rad stiffness when configured, with legacy
  `tires.front_roll_stiffness_fraction` fallback for older configs
- calculate ground-effect aero from current compressed front/rear ride height
  and rake, including exponential ride-height gain, tunnel-balance CoP shift,
  braking CoP migration, undertray stall/bottoming reduction, asymmetric
  bottoming CoP shift, a small instant current-platform aero-load component,
  and forward CoP migration when the nose rides lower than the rear, alongside
  heading-coupled speed-squared aerodynamic yaw damping that keeps normal
  straight/corner stability but fades toward a configured floor when the car is
  sideways to its velocity, and
  switchable config-backed Speedway and Road Course aero presets that modify
  downforce, drag, base front aero balance, braking CoP shift, stall height, and
  stall reduction
- apply track banking/camber and road grade to physics through lateral and
  longitudinal gravity decomposed relative to the track tangent, with
  bank/grade-adjusted normal-load calculations
- sample asphalt/apron/curb/grass independently at all four tire contact patches,
  then apply per-wheel lateral grip, grass-specific longitudinal grip, banking,
  road pitch, local filtered terrain height, local road normal, curb/roughness
  metadata, and rolling-resistance multipliers to the tire-force/resistance
  model; Hill wheel queries use the exact center sample's lap distance as an
  O(1) local-segment hint; the
  tire solver uses separate longitudinal and lateral friction limits so the
  default grass still slides easily while having enough straight-line bite and
  reduced grass-trap damping for keyboard throttle escape
- apply extra grass-trap rolling resistance, speed-sensitive damping, and
  warmer off-track dust puffs when the car leaves the racing surface
- contain the vehicle at the outer wall and visible inner infield wall with a
  conservative center-clearance broad phase and a yaw-aware four-corner
  bounding-box narrow phase that applies contact-point linear and yaw impulse
  response, then reset it near start/finish when requested
- time laps using ordered checkpoints, invalidate when the vehicle center enters
  grass or contacts a wall while allowing individual tire excursions, apron,
  and curbs, and record
  the best valid completed lap as an in-memory translucent ghost car with
  a compact 2048-sample, render-frame-gated pose buffer instead of
  per-physics-step storage
- show vehicle, performance, input, and raw SDL device diagnostics when the
  debug overlay is toggled on
- draw lightweight HUD backing panels, darker worn racing surface, generated
  material texture detail, shader-integrated asphalt rubber/paint markings,
  outside catch fencing, full frontstretch grandstands, a render-only IMS-style
  pit lane/frontstretch complex with pit wall, box markings, garages, Yard of
  Bricks, and a Pagoda-style infield scoring tower near start/finish, plus a
  checkered start/finish marker with a compact generated WebP texture set for
  material realism
- draw a glass-styled MoTeC-like bottom instrument cluster with rounded
  translucent panels, glow accents, prominent digital MPH/gear/RPM text,
  brake/throttle bars, tire temperature/wear/usage widgets that pulse when a
  tire exceeds high combined-slip usage, a dynamically scaled curved segmented
  RPM sweep with a small purple optimal-upshift marker and amber wheelspin
  tint under rear longitudinal slip, fuel-laps and engine-map readouts,
  generated SDF text for sharper HUD/debug
  rendering, and a HUD shader that samples the resolved HDR scene behind the
  panels with a compact 7-tap blur for a frosted refractive glass effect
- synthesize lightweight SDL3 audio at runtime with a high-revving procedural
  combustion engine layer that combines V6 firing-rate pulse shaping, multiple
  harmonic orders, smoothed RPM/throttle response, turbo whistle/hiss,
  mechanical noise, soft overdrive, off-throttle burbles/pops, tire
  scrub/squeal noise that begins below peak slip for earlier tire feedback,
  and collision impact thuds without external audio assets
- show an Escape menu for runtime keyboard steering rate tuning, front/rear
  wing adjustment, brake-bias adjustment, engine-map selection,
  automatic-transmission toggling, aero preset toggling, and record/ghost reset
- enumerate connected SDL joysticks/gamepads and show axes/buttons/hats
- map configured wheel axes/buttons with deadzones, G29-style pedal inversion,
  steering/throttle/brake gamma curves, a configurable wheel steering
  sensitivity multiplier, paddle shifting, and a provisional wheel
  camera-toggle button
- cycle Lean/Standard/Rich engine maps while driving with the `M` hotkey
- attempt SDL3 haptic force feedback from the selected wheel with constant-force
  streaming, spring fallback, rumble fallback, and clear no-op status when
  unsupported
- derive FFB force from front tire lateral forces multiplied by pneumatic and
  mechanical trail, pneumatic-trail/understeer lightening past peak slip,
  low-speed centering, steering damping, grass/apron vibration, and wall-contact
  jolts
- load graphics, input, vehicle body/suspension/tire/aero/drivetrain,
  oval-track, Hill Circuit track, and FFB settings from JSON
- optionally run a timed live render benchmark with
  `SIM_BENCHMARK_SECONDS=<seconds>`; benchmark runs automatically enter the
  selected track, then print FPS, frame time, physics time, render time, and
  physics steps/sec before exiting
- toggle fullscreen and diagnostic overlays
- package SDL3, configs, assets, and documentation inside the app bundle

## Important Limitations

- The current physics now has sprung heave/pitch/roll, four unsprung wheel
  hubs, spring/damper/ARB loads, tire relaxation length, per-wheel angular
  velocity, and ride-height-sensitive aero. It is still a purpose-built
  lightweight simulator model rather than a full finite-element tire model,
  rigid-body collision engine, or licensed commercial-grade vehicle dynamics
  solver.
- Wheel hub vertical dynamics now use deterministic per-wheel contact
  height/normal samples for banking transitions, seams, curbs, and lightweight
  road texture. There are still no scanned heightmaps, pothole assets, or
  arbitrary 3D mesh contacts.
- Brake discs are intentionally rendered as clean metallic parts without the
  previous red heat-glow presentation. There is still no physical brake thermal
  simulation.
- Tire smoke/dust and undertray sparks are renderer-side presentation effects
  derived from rear tire usage/slip, surface type, and floor-strike telemetry;
  they are not fluid, debris, or material-removal simulations. Exhaust flame
  particles and rear-exhaust heat shimmer have been removed for a more serious
  sim presentation.
- The material texture set is generated and compact, not scanned/photogrammetry
  source art. It now includes 2K asphalt/grass maps, 2K concrete/carbon maps,
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
- Skidmarks persist only for the current renderer session and use a fixed-size
  ring buffer that overwrites the oldest marks after the configurable 2048
  vertex cap is reached. They are visual tire marks, not tire-rubber
  accumulation affecting physics.
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
  compact Pacejka-lite peak/falloff curve, speed-dependent cornering stiffness,
  induced scrub drag, degressive load sensitivity, suspension-coupled dynamic
  camber gain and camber thrust, friction-circle combined-slip limiting,
  front/rear pneumatic/mechanical trail aligning moment, slip/usage-driven tire
  temperature/thermal-grip state, and a lightweight tire wear state. It still
  has no pressure, carcass modes, full suspension-geometry solver,
  contact-patch deformation, or fully validated tire test data.
- Wheel angular velocity supports wheelspin and lockup behavior. Engine RPM is
  coupled to driven wheel speed with an idle clamp for launchability; a full
  clutch, anti-stall, starter, and engine-stall simulation is not implemented.
  Fuel burn and Lean/Standard/Rich engine maps are lightweight deterministic
  approximations rather than ECU/boost/fuel-pressure simulations.
- Ground-effect downforce reacts to front/rear ride height and rake with an
  undertray stall floor and config-backed aero package presets, but it is an
  analytic model, not CFD or wind-tunnel-derived aero-map data.
- Oval wall containment uses a yaw-aware four-corner vehicle bounding box
  against the analytic outer and inner wall offsets. It prevents center-only
  wall clipping and adds contact-point yaw impulse response, but there is still
  no general arbitrary-geometry collision system.
- Runtime menu adjustments, including front/rear wing, brake bias, engine map,
  transmission mode, and aero preset, are not persisted back to JSON yet;
  restart returns to the config file defaults.
- The ghost car is an in-memory personal-best lap for the current run only; it
  uses a compact 2048-sample render-frame-gated pose buffer for memory
  efficiency and is not saved to disk between launches.
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
7. `RaceSession` creates the selected track from config and resets the vehicle
   at that track's spawn pose.
8. `MetalRenderer` creates the SDL Metal view, Metal pipelines, selected track geometry,
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
5. `RaceSession` samples the current surface, banking/camber, and grade and advances
   `Vehicle::step` zero or more times at the configured physics rate.
6. The race session resolves the outer/inner barriers, applies extra grass
   damping, advances checkpoint lap timing, and updates the render-frame-gated
   2048-sample in-memory ghost recorder/playback state.
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

- `src/app`: application lifecycle, Home/Racing state machine, pre-race
  track/setup selection, pause-menu return-to-Home flow, and fixed-timestep
  coordination
- `src/audio`: SDL3 audio-stream ownership and code-synthesized high-rev
  combustion engine, turbo/mechanical noise, off-throttle burbles, tire scrub,
  and collision sounds
- `src/platform`: SDL window ownership and resource-path lookup
- `src/config`: small dependency-free JSON config reader and typed app config
- `src/input`: semantic controls, keyboard input, SDL device enumeration, and
  provisional wheel mapping, including speed-sensitive keyboard steering rate
  and target limiting, pedal inversion, deadzones, gamma curves, direct wheel
  steering sensitivity, and the configured camera-toggle button
- `src/game`: polymorphic track interface, `OvalTrack` four-turn speedway
  geometry math, `HillCircuitTrack` procedural road-course centerline/elevation
  math, surface queries with split lateral/longitudinal grip telemetry, race
  coordination, quintic banking-easement runouts, road-course grade/camber
  samples, signed inner/outer barrier fields consumed by yaw-aware four-corner
  containment, grass-trap rolling resistance and speed-sensitive speed bleed,
  track-provided checkpoint lap timing, and compact render-frame-gated
  in-memory ghost-lap recording/playback
- `src/physics`: config-driven sprung/unsprung vehicle state, four-corner
  spring/damper/ARB suspension loads, split bump/rebound damper rates,
  tire vertical stiffness,
  load/speed-dependent transient tire relaxation, Pacejka-lite tire forces with
  peak/falloff behavior, induced scrub drag, speed-dependent cornering stiffness,
  degressive load-sensitive friction limits, suspension-coupled dynamic camber
  gain with mirrored solver-side camber thrust, solver-owned
  front/rear pneumatic/mechanical trail telemetry and yaw aligning moment,
  combined-slip tire forces with separate longitudinal/lateral friction limits,
  force-at-corner rigid-body tire torque integration, dynamic tire
  temperature/thermal-grip/wear state, dynamic wheel angular
  velocity/slip ratio, banking, fuel consumption with Lean/Standard/Rich engine
  maps, runtime wing/brake-bias setup offsets, per-wheel surface multipliers
  from wheel contact patch sampling, IMS speedway geometric gearing
  with shift cooldown, strict manual/automatic transmission modes, smooth
  manual-mode RPM limiter bounce, rear clutch-pack limited-slip differential,
  drivetrain, brake,
  current-ride-height-sensitive ground-effect aero with
  dynamic CoP migration and config-backed speedway/road-course package presets,
  load-transfer, direct wheel full-rack steering authority with input-side
  high-speed keyboard response shaping and target limiting,
  contact-offset wall impulse response, launch wheelspin-aware automatic
  upshift gating, and telemetry behavior
- `src/render`: Metal renderer, compact ImageIO/CoreGraphics texture loading,
  header-only OBJ loading, generated high-detail OBJ car/wheel/steering meshes,
  generated mipmapped WebP PBR material maps, generated skybox/smoke maps,
  procedural track/fence/grandstand/Pagoda scoring tower/HUD geometry, split
  car-body/wheel rendering, configurable 2x default MSAA HDR scene rendering,
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
  suspension rod rendering, SDF font atlas, dedicated low-cost frosted-glass
  Home screen rendering, 3D steering-wheel display text,
  HDR-sampling frosted MoTeC-style HUD geometry with dynamic circular RPM arc,
  purple upshift marker, enlarged tire temp/wear/usage widgets, enlarged
  brake/throttle fill bars, fuel/map text, soft
  smoke/dust/sparks, persistent visual skidmarks, and shaders
- `src/ui`: fixed-capacity HUD, debug, physics, lap, and device-diagnostic
  overlay plus current Escape setup/tuning-menu text with a Quit to Home action
- `src/ffb`: SDL3 haptic force-feedback backend with constant-force streaming,
  spring/rumble fallbacks, front tire pneumatic/mechanical-trail aligning
  torque, peak-slip lightening, smoothing, clipping, and unsupported-device
  no-op status
- `config`: editable graphics, input, vehicle, four-turn oval track, Hill
  Circuit track, and FFB data
- `assets/meshes`: generated Wavefront OBJ meshes for the car body, wheels, and
  animated steering wheel, including an IR-18-inspired low wedge nose, tight
  monocoque, compact tapered sidepods, blade-like front wing, compact rear
  wing, diffuser/floor strakes, suspension rods/joints, halo, helmet/visor,
  rear-size tire/rim/brake geometry with front visual scaling, and yoke wheel
  detail
- `assets/textures`: generated WebP PBR texture set with 2048x2048
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
  radii, skidmark vertex budget, active track selection, and physics timing;
  the default active track is
  `oval`, the default physics tick is 360 Hz for the stiff IR-18-style
  spring/unsprung-mass setup, the default MSAA sample count is 2, the default
  shadow map size is 2048, and the default shadow update interval is 2 frames
- `config/input_default.json`: keyboard response, provisional wheel mapping,
  camera-toggle button, pedal inversion, deadzones, wheel steering sensitivity,
  and wheel/pedal gamma curves; the default keyboard steering rates are
  `steer_rate = 3.2` and `return_rate = 5.2`
- `config/vehicle_openwheel_default.json`: vehicle body/inertia, steering,
  suspension, four-corner tire/load-transfer, drivetrain/wheel-inertia, brake,
  IMS-stacked geometric speedway gearing, automatic transmission toggle,
  Pacejka-lite tire curve shape, degressive tire load-sensitivity coefficients,
  speedway and road-course static camber setup values, front/rear suspension
  camber-gain values, tire pneumatic and mechanical trail values,
  load/speed-dependent tire relaxation length values,
  speed-dependent tire stiffness-softening values, tire wear, fuel capacity and
  burn-map values, Lean/Standard/Rich engine map multipliers,
  split bump/rebound damper coefficients, speedway and road-course aero preset
  packages, live front/rear wing setup
  scalars, ground-effect aero/CoP sensitivity, resistance parameters, and input-side
  high-speed keyboard steering response values; the default physical rack limit
  is `18 deg`, the default high-speed keyboard steering scale is `0.45` at
  `90 m/s`, the default rear differential uses `40 Nm` preload with ramp
  locking, the default 6th gear redlines at about 239 mph with the configured
  tire radius, and each 12,000 RPM upshift drops to about 9,500 RPM
- `config/ffb_default.json`: SDL haptic enable flag and physics FFB gains for
  master strength, centering, damping, pneumatic/mechanical trail aligning
  torque, understeer lightening, grass vibration, collision jolts, and max force
- `config/track_oval_default.json`: IMS-derived four-turn oval dimensions,
  50 ft straight/60 ft turn render widths, 55 ft average physics width,
  short-chute length, banking, 300 ft banking-easement runout length,
  outer/inner wall offsets, render-only pit lane/frontstretch/Pagoda/Yard of
  Bricks/start-finish gate at 304.8 m, 40 m pre-line spawn, config-driven
  checkpoint distances, lateral surface grip, terrain-height controls,
  grass longitudinal escape grip, apron resistance, wall response, spawn
  position, and render resolution
- `config/track_hillcircuit_default.json`: Hill Circuit type/name, 3.6 km lap
  length, 12 m road width, curb/runoff widths, surface grip/resistance values,
  terrain-height controls, armco barrier offsets, checkpoints, spawn distance,
  render resolution, and elevation-spine control points

Config changes require an app restart. Live reload is not implemented.
`SIM_ACTIVE_TRACK=hillcircuit` can override the active track for one launch.
Escape-menu steering-rate, wing, brake-bias, engine-map, auto-shift, and aero
preset changes are runtime-only.

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

Verified on June 19, 2026 after the per-wheel terrain contact-height and
2048 asphalt/grass texture pass:

- `TrackSample` remains trivially copyable and now carries scalar road height,
  road normal, roughness, curb height/phase, and filter-time data with no
  string/vector hot-path fields
- `RaceSession` passes four relative wheel contact heights into `Vehicle`,
  and `Vehicle` filters them into tire compression, contact velocity, normal
  load projection, and ride-height-sensitive aero platform calculations
- `game_tests` now covers disabled terrain preserving the old analytic road
  plane, banked oval left/right contact-height separation, raised Hill Circuit
  curb contact height, and curb-side-only wheel-patch height application
- `python3 scripts/generate_pbr_textures.py` regenerated the material bundle
  with 2048x2048 asphalt/grass maps; `du -sh assets/textures` reports `4.3M`
- `cmake --build build --config Release` succeeded and
  `ctest --test-dir build --output-on-failure` passed both tests
- the explicit oval benchmark reported `FPS 48.9`, `FRAME 20.44 ms`,
  `PHYS 0.079 ms`, `RENDER 19.77 ms`, and `PHYS_STEPS 358.5/s`
- the Hill Circuit benchmark reported `FPS 57.4`, `FRAME 17.41 ms`,
  `PHYS 0.094 ms`, `RENDER 16.80 ms`, and `PHYS_STEPS 359.2/s`

Verified on June 19, 2026 after the heading-coupled yaw-damping, grass
rebalance, slip-feedback HUD/audio, and skidmark-buffer trim pass:

- Fix 1 rebuilt cleanly and passed ctest after replacing rear-slip-proxy aero
  yaw damping with heading-alignment gating; config_tests covers nose-on and
  sideways alignment factors
- Fix 2 rebuilt cleanly and passed ctest after rebalancing oval and Hill grass
  to lower lateral grip, stronger longitudinal escape bite, reduced rolling
  resistance versus the prior trap value, and lower speed-sensitive grass
  damping constants
- Fix 3 rebuilt cleanly and passed ctest after wiring rear wheelspin into an
  amber RPM-arc tint, pulsing high-usage tire widgets, debug slip-ratio rows,
  and an earlier tire-scrub audio threshold
- Fix 4 rebuilt cleanly and passed ctest after reducing the fixed skidmark
  budget from 3072 to a config-backed 2048-vertex hard cap and converting
  appends to ring-buffer writes that upload only the newest 12-vertex mark

Verified on June 19, 2026 after stable-shadow, shadow-culling, and physics
hot-path optimization:

- shadow projection centers are quantized to 2048-map world texels before the
  light view is built, while the existing two-frame cached update interval is
  preserved
- static track shadow geometry is divided into startup-built contiguous ranges;
  each update submits only ranges whose full 3D AABB intersects the cached
  light frustum, with no per-frame allocation or Metal-buffer rebuild
- Hill wheel samples use a center-distance local query validated against exact
  queries at representative flat, crest, corkscrew, and hairpin locations;
  the wall broad phase preserves existing four-corner collision regressions
- before changes, clean eight-second runs measured Oval `FPS 45.8`,
  `FRAME 21.85 ms`, `PHYS 0.056 ms`, `RENDER 21.04 ms`; and Hill Circuit
  `FPS 49.4`, `FRAME 20.24 ms`, `PHYS 0.111 ms`, `RENDER 19.41 ms`
- after changes, Oval measured `FPS 46.5`, `FRAME 21.52 ms`, `PHYS 0.047 ms`,
  `RENDER 20.76 ms`, `PHYS_STEPS 359.2/s`; Hill Circuit measured `FPS 50.0`,
  `FRAME 20.00 ms`, `PHYS 0.076 ms`, `RENDER 19.31 ms`, and
  `PHYS_STEPS 359.2/s`
- the result reduced all requested measured budgets on both tracks; Hill
  physics fell approximately 32%, while the remaining frame cost is dominated
  by the full HDR main scene and post-processing rather than physics
- `cmake --build build --config Release` succeeded and
  `ctest --test-dir build --output-on-failure` passed both tests

Verified on June 19, 2026 after correcting the oval start/finish rendering:

- oval distance zero remains the center of the front straight; the configured
  Yard of Bricks, checkered marker, timing gate, and first checkpoint are
  co-located 304.8 m forward along the straight, before corner entry at
  502.92 m
- `game_tests` geometrically verifies the 304.8 m longitudinal displacement,
  zero lateral displacement, updated sector gates, and nonzero lap-timing
  origin
- the signed app bundle contains the same `304.8` m Yard of Bricks and
  `[304.8, 1310.64, 2316.48, 3322.32]` checkpoints as the source config
- JSON config files are now link dependencies, so a config-only edit reruns
  resource staging and bundle signing instead of leaving stale bundled values;
  this was verified with a no-source-change rebuild
- `cmake --build build --config Release` succeeded and
  `ctest --test-dir build --output-on-failure` passed both tests

Verified on June 19, 2026 after the Hill Circuit hot-path, center-based lap
validity, oval timing-origin, and filtered longitudinal load-transfer pass:

- `cmake --build build --config Release` succeeded after each independent
  change; final `ctest --test-dir build --output-on-failure` passed
  `config_tests` and `game_tests`
- the Hill Circuit slope-cache regression returned the identical value for
  1,000 repeated same-segment queries; game regressions also cover a
  grass-touching tire with an asphalt vehicle center, the 304.8 m oval timing
  gate, and progressive longitudinal load-transfer response
- the clean default-oval benchmark reported `FPS 59.4`, `FRAME 16.82 ms`,
  `PHYS 0.062 ms`, `RENDER 16.29 ms`, and `PHYS_STEPS 359.1/s`
- the final Hill Circuit benchmark reported `FPS 50.2`, `FRAME 19.94 ms`,
  `PHYS 0.116 ms`, `RENDER 19.11 ms`, and `PHYS_STEPS 358.4/s`, reducing the
  previous `0.682 ms` Hill physics result by approximately 83% and placing it
  about 1.9x above the oval result; the run remained render-bound on this Mac,
  so the observed Hill render variance is not attributed to the physics work

Verified on June 19, 2026 after the Home Screen and HUD readability overhaul:

- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure` passed `config_tests` and
  `game_tests`
- Unsandboxed GUI launch with
  `SIM_BENCHMARK_SECONDS=2 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  booted the new Home state and reported `FPS 59.1`, `FRAME 16.93 ms`,
  `PHYS 0.000 ms`, `RENDER 15.94 ms`, and `PHYS_STEPS 0.0/s`, confirming the
  fixed-step physics loop stays idle while the Home screen is active

Verified on June 19, 2026 after the IMS oval precision rebuild and render-only
pit lane/frontstretch complex:

- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure` passed `config_tests` and
  `game_tests`; `game_tests` covers `OvalTrack` barrier samples, track-provided
  checkpoint state, IMS oval precision dimensions, the
  304.8/1310.64/2316.48/3322.32 m oval checkpoint markers, `track.type = "oval"` loading, flat oval road-pitch
  regression, Hill Circuit config loading, asphalt/curb/grass classification,
  corkscrew descent grade, Hill Circuit checkpoint lap completion, and
  track-provided Hill Circuit spawn reset
- `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched the default oval and reported `FPS 59.8`, `FRAME 16.72 ms`,
  `PHYS 0.050 ms`, `RENDER 16.36 ms`, and `PHYS_STEPS 359.3/s`
- `SIM_ACTIVE_TRACK=hillcircuit SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched Hill Circuit and reported `FPS 59.4`, `FRAME 16.82 ms`,
  `PHYS 0.682 ms`, `RENDER 15.55 ms`, and `PHYS_STEPS 359.2/s`

Verified on June 19, 2026 after adding the oval Pagoda-style scoring tower:

- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure` passed `config_tests` and
  `game_tests`
- `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched the app and reported `FPS 57.7`, `FRAME 17.34 ms`, `PHYS 0.051 ms`,
  `RENDER 16.73 ms`, and `PHYS_STEPS 358.5/s`

Verified on June 19, 2026 after the dynamic camber-gain and per-step surface
sample pooling pass:

- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure` passed `config_tests` and
  `game_tests`; `config_tests` now covers split damper config loading,
  front/rear camber-gain config loading, and compressed-front-suspension
  dynamic negative-camber telemetry, while `game_tests` includes a
  center-asphalt/inside-tires-apron regression for per-wheel surface sampling
- `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched the app and reported `FPS 50.0`, `FRAME 20.01 ms`, `PHYS 0.050 ms`,
  `RENDER 19.22 ms`, and `PHYS_STEPS 359.1/s`

Verified on June 18, 2026 after the tire wear, fuel consumption, engine-map,
HUD, hotkey, and setup-menu pass:

- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure` passed `config_tests` and
  `game_tests`; `config_tests` now covers tire wear config/load behavior, fuel
  burn telemetry, Lean/Standard/Rich engine-map torque and burn ordering, and
  live front/rear wing aero effects
- `SIM_BENCHMARK_SECONDS=8 ./build/LightweightSim.app/Contents/MacOS/LightweightSim`
  launched the app and reported `FPS 56.6`, `FRAME 17.67 ms`, `PHYS 0.033 ms`,
  `RENDER 16.98 ms`, and `PHYS_STEPS 358.5/s`

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
  texture set, originally including 4096x4096 asphalt/grass albedo, normal,
  roughness, and height maps later resized to the current 2048x2048 defaults;
  2048x2048 concrete/carbon maps; and
  `assets/textures/skybox_ims_evening.webp`
- `cmake --build build --config Release` succeeded
- `ctest --test-dir build --output-on-failure -C Release` passed
  `config_tests` and `game_tests`; `config_tests` now includes the 360 Hz
  physics-rate check, IMS speedway 6th-gear redline, geometric 9500 RPM
  post-shift drop checks, Pacejka-lite tire/aero config load checks,
  degressive load-sensitivity config and high-load cornering regression checks,
  camber config, dynamic camber-gain telemetry, straight-line
  camber-cancellation, and camber-thrust regression checks,
  pneumatic-trail telemetry and aligning-yaw-moment regression checks,
  load-dependent relaxation-length config and telemetry regression checks,
  speedway/road-course aero package config and behavior regression checks,
  dynamic tire temperature/thermal-grip telemetry checks, a rigid-body steering/yaw
  regression check, manual shift-cooldown and manual limiter regression checks,
  and full-throttle launch/top-gear pull regression checks that prevent
  automatic wheelspin gear skipping while validating high-gear rear slip and
  RPM behavior. `game_tests`
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
- final verification after the Pacejka stiffness, high-speed steering authority, and
  3rd-gear wheelspin upshift-gate fixes rebuilt cleanly, passed `ctest`, and
  benchmarked at `FPS 59.5`, `FRAME 16.80 ms`, `PHYS 0.039 ms`,
  `RENDER 16.18 ms`, and `PHYS_STEPS 359.2/s`.
- final verification after the steering/limit-handling realism pass rebuilt
  cleanly, passed `ctest`, and benchmarked at `FPS 47.1`, `FRAME 21.24 ms`,
  `PHYS 0.038 ms`, `RENDER 20.56 ms`, and `PHYS_STEPS 358.4/s`; physics
  remained inexpensive while this run was render-bound.
- final verification after the induced tire scrub drag, speed-dependent
  cornering-stiffness softening, rear clutch-pack LSD, and render-frame-gated
  ghost recorder pass rebuilt cleanly, passed `ctest`, and benchmarked at
  `FPS 47.2`, `FRAME 21.19 ms`, `PHYS 0.037 ms`, `RENDER 20.36 ms`, and
  `PHYS_STEPS 358.4/s`; physics remained inexpensive while this run was
  render-bound.
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

## Modular Track System

The current track architecture separates the polymorphic game-facing `Track`
interface from concrete track implementations. Shared `TrackPose`,
`TrackSample`, `TrackSurface`, `CheckpointState`, and `BarrierSample` data
structures live in the common track header. `OvalTrack` owns the existing
four-turn speedway pose/sample math and keeps the current
`track_oval_default.json` config with `track.type = "oval"`. `HillCircuitTrack`
owns the procedural 3.6 km road-course centerline, elevation spline, camber,
curb/grass runoff, checkpoints, spawn, and signed barrier fields from
`track_hillcircuit_default.json`. `RaceSession` owns a `std::unique_ptr<Track>`
from the track factory, so spawn pose, lap timing, per-wheel surface sampling,
grade/camber, and wall containment go through the interface.

Hill Circuit elevation slopes are precomputed beside the spline points and
`slopeAtDistance()` caches its active segment. Nearest-centerline sampling also
searches a fixed window around the last segment, then uses precomputed fixed
segment-chunk bounds to reject every region that cannot contain a closer point.
This preserves exhaustive-search results for teleports and spatially close
track sections while directly reusing POD samples for duplicate barrier
queries. These caches add no allocation to the 360 Hz path.

Current base interface:

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

Completed architecture:

- The current concrete oval implementation is `OvalTrack`; the config remains
  `track_oval_default.json`.
- The second concrete implementation is `HillCircuitTrack`; the config is
  `track_hillcircuit_default.json` and the user-facing name is `Hill Circuit`.
- Shared track samples, surfaces, checkpoint state, and signed barrier fields
  are common data types.
- `RaceSession` owns a `std::unique_ptr<Track>` factory result selected from
  `track.type`, and lap timing, spawn selection, surface multipliers, and
  barrier containment are track-interface calls.
- Inner/outer wall containment uses `BarrierSample::signedDistanceM` and
  track-provided barrier normals rather than hard-coded oval offsets in
  `RaceSession`.
- `MetalRenderer` builds static track geometry for both concrete tracks and
  draws them through the same material, shadow, HDR, bloom, and HUD pipelines.

Remaining milestones:

- Move static track render mesh generation fully behind a reusable
  `TrackRenderGeometry` data object if future tracks need authorable mesh
  assets rather than concrete renderer-side generation helpers.
- Add per-wheel terrain-height sampling and curb height profiles; current
  tracks still expose smooth analytic road planes for suspension travel.
- Add persistent user selection of `simulation.active_track` instead of editing
  JSON or using `SIM_ACTIVE_TRACK` at launch.

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
