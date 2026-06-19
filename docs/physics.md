# Physics

## Current Model

The current prototype uses a lightweight sprung/unsprung four-corner vehicle
model, not direct arcade rotation and not a full commercial vehicle-dynamics
solver. The car has world-space linear velocity, yaw angle, yaw rate, chassis
heave, pitch, roll, four wheel-hub vertical states, and per-wheel angular
velocity. Each tire contact patch calculates its own local longitudinal speed,
lateral speed, relaxed slip state, dynamic slip ratio, normal load, friction
limit, combined-slip force, tire usage, and yaw moment.

Each wheel starts with a compact Pacejka-lite tire curve. The curve keeps the
configured stiffness around zero slip, reaches peak force near the configured
peak slip, then gently falls away after the peak:

```text
pacejka_lite = D * sin(C * atan(B*x - E*(B*x - atan(B*x))))
normalized_load = normal_load / tires.load_reference_normal_n
load_efficiency = clamp(
    1 - tires.load_sensitivity_coeff * (normalized_load - 1),
    tires.load_sensitivity_min_efficiency,
    1)
friction_limit = tire_friction_coefficient * surface_grip * normal_load * load_efficiency
```

`D` is the lateral or longitudinal friction limit, `B` is derived from tire
stiffness and peak slip, `C` is `tires.curve_shape_factor`, and `E` is
`tires.curve_curvature_factor`. `tires.post_peak_falloff` trims force after
the peak so the edge of grip is progressive instead of an abrupt clamp.
The solver also computes a config-backed minimum effective `B` factor from
`tires.pacejka_min_stiffness_factor`, `tires.pacejka_peak_force_target`, and
`tires.pacejka_max_stiffness_factor`. If a tire stiffness value is too low to
let the Pacejka-lite curve approach its friction limit near the configured peak
slip, the internal curve factor is boosted for that step instead of silently
flattening the tire. The default IR-18 tire rates are high enough for this to
act as a safety guard rather than normal tuning.

Cornering stiffness is softened by wheel rotational speed before the
Pacejka-lite curve is evaluated:

```text
omega_norm = abs(wheel_angular_velocity) / tires.stiffness_speed_ref_rps
stiffness_factor = 1 / (1 + tires.stiffness_speed_softening * omega_norm^2)
effective_cornering_stiffness = static_cornering_stiffness * stiffness_factor
```

This approximates high-speed carcass growth and contact-patch softening without
adding tire state. At low speed the multiplier is effectively one; at speedway
wheel speeds the response is subtly softer.

Longitudinal drive and brake demand then share the same wheel grip budget
through a combined-slip friction ellipse. Lateral grip uses the full tire
friction limit while longitudinal grip is scaled by
`tires.longitudinal_grip_fraction`:

```text
longitudinal_limit = friction_limit * tires.longitudinal_grip_fraction
lateral_limit = friction_limit
usage = sqrt((longitudinal_demand / longitudinal_limit)^2
             + (lateral_demand / lateral_limit)^2)
if usage > 1, scale both force components down by 1 / usage
```

The combined-slip resolver keeps longitudinal and lateral demand separate when
returning scaled forces. This is important for high-speed coasting stability:
off-throttle wheels can still generate lateral force instead of accidentally
collapsing lateral grip to the longitudinal demand.

Lateral slip also produces induced scrub drag before the combined-slip ellipse
is resolved:

```text
induced_drag = -lateral_demand * sin(relaxed_slip_angle)
longitudinal_demand += induced_drag
```

The drag therefore competes for the same tire friction budget instead of being
an extra force. Sliding through a corner now bleeds vehicle speed and makes
overdriven understeer cost lap time.

Tire temperature maps to grip through a Gaussian peak window:

```text
delta = tire_temperature_c - tires.thermal_optimal_c
thermal_grip = tires.thermal_grip_min
               + (1 - tires.thermal_grip_min)
                 * exp(-(delta^2) / (2 * tires.thermal_window_c^2))
```

This makes cold tires greasy, peaks grip near the configured target
temperature, and reduces grip again when tires are overheated.

Each tire also carries a lightweight remaining-life value from `1.0` fresh to
`0.0` worn. Wear loss is linear in sliding work and wheelspin work, using the
same per-wheel slip and usage telemetry that drives temperature:

```text
usage_work = max(0, tire_usage - 0.72)^2 * speed_weight
slip_work = max(0, lateral_slip_load - 0.85)^2 * speed_weight
spin_work = max(0, longitudinal_slip_load - 1.0)^2 * speed_weight
tire_wear -= dt * (tires.wear_sliding_rate_per_work * (usage_work + slip_work)
                   + tires.wear_wheelspin_rate_per_work * spin_work)
```

The tire's friction input is multiplied by a wear grip scale that linearly
interpolates from `1.0` at full life to `tires.wear_min_grip` at zero life.
Cruising wear is intentionally tiny; lockups, wheelspin, and sustained sliding
are what visibly move the HUD tire-life bars.

Setup camber and suspension camber gain contribute camber thrust before the
combined-slip limiter:

```text
dynamic_bump_travel = suspension_compression - static_spring_compression
physical_camber = setup_camber
                  - suspension.camber_gain_rad_per_m * dynamic_bump_travel
camber_thrust = tires.camber_stiffness_n_per_rad
                * solver_mirrored_camber_angle_rad
                * normal_load / tires.load_reference_normal_n
lateral_demand = lateral_slip_force + camber_thrust
```

The configured front/rear camber values are physical static setup camber. The
front and rear kinematic gain values add negative camber when a hub moves into
bump and reduce it in droop. The result is mirrored left/right only at the tire
solver boundary so straight-line camber thrust cancels instead of creating
artificial drift. The road-course setup uses more aggressive static camber than
the speedway setup, and the live physical camber per tire is exposed in
`VehicleState`.

Front and rear tires compute pneumatic trail in the tire solver:

```text
slip_normalized = abs(relaxed_slip_angle) / tires.lateral_peak_slip_angle_deg
pneumatic_trail = clamp(
    tires.pneumatic_trail_max_m * (1 - slip_normalized^2),
    -tires.pneumatic_trail_max_m * 0.3,
    tires.pneumatic_trail_max_m)
aligning_trail = pneumatic_trail + tires.mechanical_trail_m
aligning_moment = -aligning_trail * tire_lateral_force
```

Rear tire trail uses the same configured maximum pneumatic and mechanical
trail values with reduced rear scaling, giving moderate rear slip a restoring
yaw contribution instead of letting oversteer rotate without self-aligning
resistance. Each wheel's aligning moment is summed into chassis yaw along with
the normal `r x F` tire force moments. The same solver-derived front trail is
exposed on `VehicleState` for force feedback, so FFB does not need to own a
separate front tire-trail model after physics has stepped.

Relaxed lateral and longitudinal slip use a live relaxation length instead of
one fixed value:

```text
load_scale = normal_load / tires.load_reference_normal_n
speed_factor = 1 / max(0.1, abs(tire_longitudinal_speed) / 30)
relaxation_length = clamp(
    tires.relaxation_length_base_m * load_scale * speed_factor,
    tires.relaxation_length_min_m,
    tires.relaxation_length_max_m)
tau = relaxation_length / max(0.01, abs(tire_longitudinal_speed))
relaxed_slip += (instantaneous_slip - relaxed_slip) * (1 - exp(-dt / tau))
```

For longitudinal force, the response speed also considers wheel surface speed
so launch wheelspin and brake lockup can build progressively even when road
speed is very low. The relaxed slip ratio is stored per wheel and is the value
fed into the longitudinal Pacejka-lite curve. This makes high-load tires build
force more gradually while preserving faster response where load and speed are
lower. The live per-wheel relaxation lengths are exposed in telemetry.

The front wheel forces are rotated by steering angle before forces and yaw
moments are integrated. `Vehicle::step` maps normalized steering input directly
to the configured physical rack limit, so full input always reaches
`steering.max_road_wheel_angle_deg` even at speed. Keyboard steering remains
speed-aware in `InputManager`: `steering.high_speed_steer_scale` and
`steering.steer_speed_threshold_mps` slow how quickly keyboard input reaches
full lock, but they no longer reduce the final road-wheel angle. Wheel devices
still pass through the calibrated input path and then into the same full-rack
physics mapping. The default car is rear-drive, with optional front-drive
fraction kept as a tuning parameter for experimentation. Braking force is split
by brake bias and then per wheel.

## Drivetrain And Brakes

The old flat `engine_force_n` placeholder has been replaced by a small
config-driven drivetrain:

- engine torque in Newton-meters
- wheel radius
- final drive ratio
- six scalar gear ratios
- drivetrain efficiency
- front/rear drive split
- rear clutch-pack limited-slip differential with preload and ramp locking
  torque, plus a load-biased fallback for older configs
- idle, redline, shift-up, and shift-down RPM
- optional automatic shifting

The torque curve is intentionally lightweight and config-driven. Up to eight
fixed-size `powertrain.torque_curve_knots` define normalized RPM and normalized
torque. The default curve is weak below the powerband, flat/strong from the
upper midrange through peak power, and drops to zero at redline:

```text
torque_multiplier = piecewise_linear(powertrain.torque_curve_knots, rpm / redline_rpm)
engine_torque = powertrain.engine_torque_nm * torque_multiplier * engine_map_torque_multiplier
```

It is not an engine dyno simulation, but it gives gearing and RPM real meaning
without heap allocation or per-step table parsing.

Fuel is tracked in gallons on `VehicleState`. The drain rate is based on RPM,
throttle load, and the active engine map:

```text
load_factor = fuel_idle_load_factor + (1 - fuel_idle_load_factor) * throttle
rpm_factor = fuel_idle_rpm_factor + (1 - fuel_idle_rpm_factor) * rpm / redline
fuel_burn = fuel_burn_gal_per_s_at_redline
            * engine_map_fuel_multiplier
            * load_factor
            * rpm_factor
```

The map states are Lean, Standard, and Rich. Lean reduces torque and burn, Rich
adds a small torque gain and extra burn, and Standard is the baseline. The HUD
uses the smoothed average burn rate and current lap-length context to estimate
fuel laps remaining.

The default speedway gearing and low-drag aero package are stacked for
Indianapolis: 6th gear redlines at about 239 mph with the configured 0.343 m
tire radius, while 4th-6th are close enough to keep the engine near 12,000 rpm
during speedway running and drafting. The limiter starts close to redline via
configurable RPM margins so it does not bleed top-gear power hundreds of RPM
early.

Brake input is gamma-shaped, split by `brakes.brake_bias`, and constrained by
the same tire friction budget as steering and throttle. Heavy brake while
turning can therefore consume front grip and reduce steering response.

The default rear-drive torque split uses a compact clutch-pack LSD:

```text
wheel_speed_diff = rear_left_omega - rear_right_omega
locking_torque = powertrain.lsd_preload_nm
                 + powertrain.lsd_ramp_factor * abs(rear_drive_torque)
lsd_torque = clamp(locking_torque * wheel_speed_diff * powertrain.lsd_sensitivity,
                   -locking_torque,
                    locking_torque)
rear_left_torque  = rear_drive_torque * 0.5 - lsd_torque * 0.5
rear_right_torque = rear_drive_torque * 0.5 + lsd_torque * 0.5
```

The preload term acts even with little or no throttle, while the ramp term adds
lock under power. Legacy configs without any LSD keys retain the old
`powertrain.differential_load_bias` rear split.

## Suspension Damping

Each corner uses spring force from suspension compression plus a directional
damper force from relative hub/mount shaft speed:

```text
compression_rate = hub_velocity - chassis_mount_velocity
damper = compression_rate >= 0
         ? suspension.*_damper_bump_n_per_mps
         : suspension.*_damper_rebound_n_per_mps
spring_force = spring_rate * compression + damper * compression_rate
```

Positive compression rate means the wheel hub is moving into bump relative to
the sprung chassis; negative compression rate means rebound/extension. Bump
damping controls compression resistance, platform support, and weight-transfer
speed. Rebound damping controls how quickly the wheel and chassis separate
after compression, which affects wheel return rate and contact-patch recovery.
The legacy `front_damper_n_per_mps` and `rear_damper_n_per_mps` fields remain
fallbacks for older configs, but the default setup uses split bump/rebound
values with rebound stiffer than bump.

## Weight Transfer And Aero

Static axle load comes from mass, effective gravity, and
`body.front_weight_fraction`.
Longitudinal load transfer changes front/rear normal load. The instantaneous
acceleration-derived target is passed through a first-order suspension/
compliance response before it reaches the tire normal-load distribution:

```text
instantaneous_transfer = mass * longitudinal_acceleration
                         * center_of_mass_height / wheelbase
alpha = 1 - exp(-dt / suspension.longitudinal_load_transfer_tau_s)
filtered_transfer += alpha * (instantaneous_transfer - filtered_transfer)
```

Positive acceleration moves load rearward. Braking moves load forward. The
default 0.08 s response makes initial braking and launch load build through the
platform rather than appearing at full magnitude in one 360 Hz step.

Lateral load transfer changes left/right normal load:

```text
transfer = mass * lateral_acceleration * center_of_mass_height / track_width
```

By default, the front/rear share of lateral load transfer is derived each step
from setup roll stiffness:

```text
front_roll_stiffness = 0.5 * front_spring_rate * track_width^2
                       + suspension.front_arb_nm_per_rad
rear_roll_stiffness  = 0.5 * rear_spring_rate * track_width^2
                       + suspension.rear_arb_nm_per_rad
front_roll_fraction  = front_roll_stiffness
                       / (front_roll_stiffness + rear_roll_stiffness)
```

Legacy configs without both ARB roll-stiffness fields fall back to
`tires.front_roll_stiffness_fraction`. Tire grip uses degressive load
sensitivity: `tires.load_sensitivity_coeff`,
`tires.load_sensitivity_min_efficiency`, and
`tires.load_reference_normal_n` make a heavily loaded tire generate less force
per Newton than a lightly loaded tire. This is deliberately more important on
banked high-downforce speedway corners, where outside tires can carry several
times the static corner load.

Base downforce is quadratic with speed, then adjusted by current compressed
front/rear ride height, rake, tunnel balance, braking, and bottoming/stall
asymmetry:

```text
base_downforce = speed^2 * aero.downforce_n_per_mps2
downforce = base_downforce * ground_effect_multiplier(front_ride_height, rear_ride_height)
```

`aero.front_downforce_fraction` is shifted by rake so a lower front ride height
can move aerodynamic balance forward. `aero.ride_height_balance_sensitivity`
lets front/rear tunnel strength move CoP as the platform compresses, and
`aero.brake_cop_shift` moves CoP forward under braking for sharper turn-in.
`aero.stall_cop_shift` shifts balance when the front or rear floor bottoms,
and `aero.instant_load_fraction` applies a small current-platform aero-load
component directly to tire normal loads so high-speed platform changes are felt
without waiting entirely for suspension lag. Aerodynamic drag is also quadratic
with speed and acts longitudinally against vehicle motion.

Runtime front/rear wing settings are applied as setup offsets on top of the
active aero preset. More front wing shifts the center of pressure forward and
raises effective front cornering stiffness. More rear wing increases total
downforce, rear cornering authority, and drag, creating the expected stability
versus top-speed tradeoff. These settings are live-adjustable from the Escape
menu and are not persisted to JSON.

Aerodynamic yaw damping adds a stabilizing yaw moment that rises with speed
squared and with the squared cosine of the angle between body heading and
velocity heading:

```text
yaw_alignment = cos(velocity_heading - body_heading)^2
yaw_scale = aero.yaw_damping_rear_slide_min_scale
            + (1 - aero.yaw_damping_rear_slide_min_scale) * yaw_alignment
yaw_damping = aero.yaw_damping_nm_per_rad_s
              * speed^2 / aero.yaw_damping_reference_speed_mps^2
              * yaw_scale
              * yaw_rate
yaw_moment -= yaw_damping
```

At parking-lot speeds this term is effectively absent. At speedway speeds it
helps the car resist and settle small yaw perturbations without changing the
360 Hz fixed-step model or adding state. Normal small yaw angles retain nearly
full damping, while a sideways spin collapses toward the configured floor
`aero.yaw_damping_rear_slide_min_scale`. Rear breakaway is therefore governed by
the tire curve, combined-slip budget, heading geometry, and rigid-body moments
rather than by an aero damping clamp that catches the slide.

The active aero package is selected by `Vehicle::setAeroPreset`. The configured
`aero_presets.speedway` and `aero_presets.road_course` packages independently
set downforce, drag, base front downforce fraction, brake CoP shift, stall
height, and stall reduction. Speedway mode is lower-drag and more rear-biased;
road-course mode adds downforce and front aero balance for sharper braking and
turn-in.

## Banking And Grade

Track banking and road grade now contribute to the planar physics model.
`RaceSession` passes the sampled banking angle and road pitch angle into
`Vehicle::step`.

The car receives a lateral gravity force from banking:

```text
gravity_lateral_force = -mass * gravity * sin(bank_angle)
```

Normal-load calculations use a bank-adjusted effective gravity term:

```text
effective_gravity = gravity * cos(bank_angle)
                    + (longitudinal_speed * yaw_rate) * sin(bank_angle)
```

This is still not a full 3D suspension/contact model, but it lets the oval
banking support the car more realistically. The centripetal term uses the same
sign as the car's cornering direction so normal load rises when the car is
supported by the banking instead of being accidentally reduced.

Track height uses the inside edge of the asphalt road as the banking pivot.
The apron and infield stay flat at `Y = 0`; only asphalt outside the inner road
edge gains banked height. This avoids the previous centerline-height jump that
made corner entry and exit feel like a vertical hump.

Track samples also expose lightweight per-wheel terrain contact data:
`road_height_y_m`, road normal, surface roughness, and curb height/phase.
These are deterministic procedural values, not scanned assets. The oval uses
the analytic banking height plus subtle seam/undulation waves in lap-distance
space. The Hill Circuit uses the elevation spline plus lateral camber, with a
raised procedural curb profile only inside the curb band. `RaceSession`
subtracts the vehicle-center smooth analytic road height from each wheel sample
before handing contacts to `Vehicle::step`, so the physics sees local patch
height relative to the car instead of absolute world elevation. Renderer car
placement and ghost placement keep using the smooth height, letting the
suspension filter carry the roughness instead of visually teleporting the
whole car over every procedural seam.

The vehicle filters each wheel's local contact height before using it in the
vertical tire model:

```text
alpha = 1 - exp(-dt / terrain.contact_height_filter_tau_s)
filtered_road_height += alpha * (target_road_height - filtered_road_height)
tire_compression = static_tire_compression - hub_travel
                   + filtered_road_height
tire_force = tire_vertical_stiffness * tire_compression
             - tire_vertical_damping
               * (hub_velocity - filtered_road_height_velocity)
normal_load = tire_force / road_normal.y
ride_height = static_ride_height + chassis_mount_travel
              - filtered_road_height
```

The existing banking/grade gravity terms remain active; contact normals are
used for tire vertical response and normal-load projection rather than as a
second gravity model. Disabling `terrain.terrain_height_enabled` keeps the old
analytic smooth-plane behavior for physics while preserving the renderer's
track height.

Bank angle now uses quintic smoothstep easements centered on turn entry and
exit. With the default `geometry.bank_transition_runout_m = 50.0`, banking
starts blending in 50 m before turn entry, reaches full banking 50 m into the
corner, then blends out symmetrically at corner exit. This gives continuous
height/roll acceleration and avoids abrupt entrance/exit humps.

Road-course grade is injected as a longitudinal gravity component:

```text
gravity_longitudinal_force = -mass * gravity * sin(road_pitch_angle)
effective_gravity *= cos(road_pitch_angle)
```

Positive grade resists the car uphill; negative grade accelerates it downhill
and slightly reduces normal load. The Hill Circuit elevation spline uses this
path to make downhill braking zones and the corkscrew-style drop affect load
transfer without adding new vehicle state.

## Track And Surfaces

The procedural stadium oval is approximately 2.5 miles around its centerline
with IMS-derived dimensions: 3,300 ft straights, 660 ft short chutes, the
mathematically correct 256.135 m centerline turn radius, 55 ft average physics
road width, 20 ft apron, close outer SAFER-wall spacing, and a 300 ft quintic
banking runout. The renderer widens the visible road from 50 ft on straights to
60 ft in turns, but physics still uses the average width until per-segment oval
surface widths are promoted into the hot path.
The Hill Circuit is a 3.6 km counterclockwise natural-terrain road circuit with
11 turns, curb strips, grass runoff, armco-style barriers, and a 55 m-class
peak-to-valley elevation profile. Banking/camber, grade, and local
deterministic contact height contribute gravity terms, effective-normal-load
adjustments, tire vertical response, aero ride-height variation, and renderer
road height/orientation.

The track classifies each queried point as asphalt, apron, curb, or grass using the
`TrackSurface` enum; the config path is parsed once at startup and the fixed
physics loop does not compare surface strings. `RaceSession` samples all four
wheel contact patch positions each step and passes per-wheel lateral grip,
longitudinal grip, rolling resistance, banking, road pitch, tangent data,
relative road height, road normal, and curb/roughness metadata into
`Vehicle::step`. If one side straddles the apron while the car center remains
on asphalt, only those tires lose grip, gain drag, and ride on that surface's
contact height.

Each surface applies config-driven lateral-grip and rolling-resistance
multipliers. Grass also has a separate longitudinal grip multiplier: it keeps
low cornering grip so the car slides easily, but has enough straight-line tire
bite to escape with digital throttle. `RaceSession` then applies extra
grass-trap rolling resistance per grass-contact tire plus speed-sensitive
damping scaled by grass contact fraction, so fast off-track excursions bleed
speed aggressively without stranding the car at low speed. Curbs and the apron
remain valid for timing. Lap validity samples only the vehicle center, so a
single tire touching grass does not invalidate the lap while per-wheel grass
physics still applies; a lap is invalidated once the vehicle center reaches
grass or the car contacts a barrier. The
outer wall and visible inner infield wall use yaw-aware four-corner
bounding-box collision, contact-point linear/yaw impulse response, and low
restitution. A conservative center-clearance broad phase skips corner sampling
when the vehicle's complete collision circle is clear of both barriers; near a
wall, the original iterative four-corner queries and impulses remain active.
Hill Circuit wheel contact patches use the exact vehicle-center lap distance as
a local centerline hint, avoiding a circuit-wide nearest-point search without
changing the sampled branch of track beneath the car.

Lap timing starts after crossing start/finish and requires four ordered
checkpoints, preventing shortcut laps. Oval distance zero is the center of the
front straight. The config-driven Yard of Bricks/start-finish gate is 304.8 m
forward along the racing direction, followed by gates at 1310.64 m, 2316.48 m,
and 3322.32 m. `LapTimer` consumes the track-provided
start/finish distance rather than assuming the geometric lap origin is the
timing line.

## Units

- distance: meters
- time: seconds
- mass: kilograms
- force: Newtons
- angles: radians internally
- telemetry speed: meters/second, displayed as kilometers/hour

## Fixed Step

Physics runs at `simulation.physics_hz` from `config/graphics_default.json`,
with 360 Hz as the default. Render timing does not alter the physics timestep.

## Known Simplifications

- Suspension now receives per-wheel deterministic contact heights/normals for
  banking transitions, curbs, seams, and surface undulation. There are still no
  scanned heightmaps, pothole assets, or arbitrary 3D mesh contacts.
- The tire model has load/speed-dependent lateral and longitudinal relaxation
  length, dynamic relaxed slip ratio, combined-slip limiting,
  suspension-coupled camber thrust, lightweight tire wear, and a lightweight
  slip/usage-driven tire temperature and thermal grip state. Pneumatic trail is
  modeled as a compact self-aligning moment for the front and rear tires. There
  is still no pressure, carcass modes, full suspension geometry solver, or
  contact-patch deformation model.
- Brake discs are currently clean metallic render parts, not thermal brake
  simulations.
- Tire smoke/dust and undertray sparks are renderer-side presentation effects,
  not particle/fluid/debris simulations. Exhaust flame and rear-exhaust heat
  shimmer are intentionally not rendered.
- Lap invalidation uses the vehicle center surface rather than polygon/track-limit
  rules for every tire contact patch.
- No general collision shapes; only the oval outer and inner wall loops are
  active, using a yaw-aware vehicle bounding box
- The engine curve is intentionally simple and not based on measured engine data

These are intentional current-model limits. The vehicle already
produces momentum, understeer/oversteer balance, per-wheel load transfer,
sprung chassis attitude, combined-slip behavior, brake-bias effects,
ride-height-sensitive aero load, drag, gearing, wheelspin/lockup telemetry,
lateral slip, and braking behavior while keeping the vehicle module
replaceable.
