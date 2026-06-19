# Architecture

`App` owns subsystem lifetime and coordinates a render-frame loop. `GameLoop`
accumulates wall-clock time and exposes fixed 360 Hz simulation steps. Input is
sampled once per rendered frame and applied to every pending fixed step;
rendering interpolates the previous and current vehicle transforms.

`InputManager` produces semantic `InputActions`. Keyboard and wheel axes are
mapped there, so raw device values do not enter vehicle physics directly.
Keyboard steering rate is scaled down as vehicle speed rises. Wheel axes apply
deadzone filtering, pedal inversion, and gamma curves from
`config/input_default.json`. `SDLJoystickInput` owns opened SDL joystick
handles and keeps fixed-size diagnostic snapshots. Hotplug events trigger
re-enumeration.

`Vehicle` is the current replaceable physics implementation. It is configured
through `VehicleConfig` and exposes telemetry for speed, tire slip, per-wheel
loads, tire usage, tire temperature/wear, drivetrain force, fuel burn, braking
force, ride heights, hub travel, chassis heave/pitch/roll, drag,
ride-height-sensitive downforce, RPM, gear, dynamic wheel angular velocity, and
visual wheel rotation. It is now a lightweight sprung/unsprung four-corner
model with filtered per-wheel terrain-contact heights/normals,
banked-track lateral gravity and effective-normal-load adjustment,
degressive load-sensitive tire limits, suspension-coupled dynamic camber gain
with mirrored solver-side camber thrust,
solver-owned front/rear pneumatic/mechanical trail aligning moment,
load/speed-dependent tire relaxation length, dynamic slip ratio, Pacejka-lite
tire force curves, tire wear, suspension spring/damper/anti-roll-bar loads,
bi-linear bump/rebound damping,
fuel-backed Lean/Standard/Rich engine maps, IMS-stacked gearing, runtime wing
and brake-bias setup offsets, and analytic current-platform ground-effect aero
with config-backed Speedway/Road Course package presets and CoP migration. The
implementation remains compact and replaceable rather than a full commercial
vehicle-dynamics solver.

`Track` is the polymorphic game-facing track interface. `OvalTrack` owns the
four-turn speedway centerline, banking, surface, checkpoint, spawn, and signed
inner/outer barrier-field math. `HillCircuitTrack` owns the procedural
3.6 km natural-terrain road circuit centerline, elevation spline, variable
camber, curb/grass runoff surfaces, armco-style barrier fields, spawn, and
checkpoint state. `RaceSession` owns a `std::unique_ptr<Track>` produced by
track config and samples the track center and all four wheel contact patches
during fixed physics steps, applies per-wheel surface multipliers, passes
banking, road pitch, and relative contact-patch height into vehicle physics,
applies yaw-aware four-corner wall
containment from track-returned `BarrierSample` fields, applies grass-trap
damping, records the current valid lap into a fixed-size ghost buffer, and
advances `LapTimer` through track-provided checkpoint state.
`TrackSample` remains a trivially copyable POD hot-path result: surface type is
stored as an enum, contact height/normal/roughness are scalar fields, and no
string/vector allocations occur during physics steps.
Hill wheel-patch queries use the exact center sample's lap distance as an O(1)
local-segment hint. Wall collision first compares center barrier clearance with
the vehicle's full collision radius; the exact iterative four-corner solver is
only entered when contact is possible.
The Metal renderer dispatches static track mesh generation by concrete track
type, then draws both tracks through the same world material, HDR, shadow, and
post-processing pipelines.

`SoundManager` owns the SDL3 audio stream. It does not load audio files; it
synthesizes a high-revving combustion engine layer from vehicle RPM/throttle
using V6 firing-rate pulse shaping, harmonic orders, turbo/mechanical noise,
soft overdrive, and off-throttle burbles, plus tire scrub/squeal noise and
impact thuds from vehicle/input/race telemetry.

`MetalRenderer` owns the SDL Metal view, `CAMetalLayer`, sky/world/FX/UI/text
pipelines, static procedural meshes, dynamic smoke vertices, configurable 2x
default MSAA HDR/depth scene targets, resolved HDR/bloom/shadow targets, a
configurable 2048x2048 default cached sun shadow map with configurable local
frustum/light placement, texel-grid-snapped shadow centers, static
ground-shadow draw ranges with exact light-frustum AABB culling, and generated
SDF font texture. It builds a
screen-space procedural sky pass, the oval, racing surface
detail, variable-width IMS oval asphalt, pit lane/frontstretch geometry,
catch fencing, grandstands, Pagoda-style scoring tower, car body mesh,
animated steering wheel mesh, and reusable wheel mesh from code and generated
OBJ assets. The pit lane, wall, box markings, garages, and Yard of Bricks are
baked into the static world vertex buffer, so they do not add render passes or
per-frame CPU work. World vertices carry material tags and local positions so
the Metal
shader can combine mipmapped albedo/normal textures with deterministic shader
detail: asphalt groove/edge markings, contact-shadow grounding, livery
stripes/number masks, pearl metallic paint, multi-layer clearcoat, carbon
weave, tire scuffing, procedural brick shading for the start/finish strip,
metal highlights, visor reflections, brake-light emission, clean metallic
brake-disc shading, smoke/spark materials, and grandstand variation. Exhaust
flame meshes and rear-exhaust heat shimmer are not rendered.

The renderer draws the generated sky first, then depth-tested world geometry,
the car body, animated steering wheel, four transformed smooth-normal wheels,
a wheel-mounted 3D SDF telemetry display, a translucent best-lap ghost car when
available, and blended smoke/spark effects. It resolves the configurable 2x
default MSAA HDR scene, extracts bright energy through half-precision
half/quarter-resolution bloom targets, then composites the config-weighted
two-level bloom, reduced center-protected speed blur, HDR sharpening, tone
mapping, lens grading, glass-style HUD mesh with configurable blur/refraction
radii, and SDF text.
The camera is stateful inside the renderer: it supports fixed-distance chase
and chassis-locked cockpit modes, uses a 50-degree chase FOV and 55-degree
cockpit FOV, locks chase translation directly to the car to avoid high-speed
follow lag, places the cockpit eye rearward/upward inside the monocoque for a
seated over-wheel sightline, keeps chase view stable on clean asphalt by
default, retains only configurable bank/acceleration roll hints, and applies
configurable apron/grass/slip/wall-contact shake after startup settling. The UI
pass draws rounded
translucent panels, glow accents, a segmented curved RPM sweep, Escape-menu
text, and telemetry
text. UI/text/effect vertices use fixed-capacity buffers rather than allocating
each frame.

`ForceFeedbackManager` owns the SDL3 haptic backend. It opens haptics from the
configured active joystick, attempts constant-force streaming first, then spring
and rumble fallbacks, and otherwise reports a no-op unsupported status. The
requested force is generated from vehicle telemetry: front lateral force,
front pneumatic/mechanical trail, front-slip understeer lightening, low-speed
centering, steering-rate damping, grass/apron vibration, and wall-contact
jolts.
