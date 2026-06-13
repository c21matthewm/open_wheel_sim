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

Longitudinal drive and brake demand then share the same wheel grip budget
through a combined-slip friction ellipse:

```text
usage = sqrt((longitudinal_demand / limit)^2 + (lateral_demand / limit)^2)
if usage > 1, scale both force components down by 1 / usage
```

The combined-slip resolver keeps longitudinal and lateral demand separate when
returning scaled forces. This is important for high-speed coasting stability:
off-throttle wheels can still generate lateral force instead of accidentally
collapsing lateral grip to the longitudinal demand.

Static setup camber contributes camber thrust before the combined-slip limiter:

```text
camber_thrust = tires.camber_stiffness_n_per_rad
                * effective_camber_angle_rad
                * normal_load / tires.load_reference_normal_n
lateral_demand = lateral_slip_force + camber_thrust
```

The configured front/rear camber values are mirrored left/right so straight-line
camber thrust cancels instead of creating artificial drift. The road-course
setup uses more aggressive static camber than the speedway setup, but camber is
still a setup constant rather than a suspension-kinematic value.

The front wheel forces are rotated by steering angle before forces and yaw
moments are integrated. Keyboard steering also has two stability assists:
`InputManager` slows keyboard steering rate as vehicle speed rises, and
`Vehicle::step` scales the maximum road-wheel angle down toward
`steering.high_speed_steer_scale` at `steering.steer_speed_threshold_mps`.
Wheel devices still pass through the calibrated input path, but the final
front-wheel angle is constrained by the same high-speed physics clamp. The
default car is rear-drive, with optional front-drive fraction kept as a tuning
parameter for experimentation. Braking force is split by brake bias and then
per wheel.

## Drivetrain And Brakes

The old flat `engine_force_n` placeholder has been replaced by a small
config-driven drivetrain:

- engine torque in Newton-meters
- wheel radius
- final drive ratio
- six scalar gear ratios
- drivetrain efficiency
- front/rear drive split
- simple load-biased differential split
- idle, redline, shift-up, and shift-down RPM
- optional automatic shifting

The torque curve is intentionally lightweight: a smooth midrange bump with a
redline cut. It is not an engine dyno simulation, but it gives gearing and RPM
real meaning.

The default speedway gearing is stacked for Indianapolis: 6th gear redlines at
about 239 mph with the configured 0.343 m tire radius, while 4th-6th are close
enough to keep the engine near 12,000 rpm during speedway running and drafting.

Brake input is gamma-shaped, split by `brakes.brake_bias`, and constrained by
the same tire friction budget as steering and throttle. Heavy brake while
turning can therefore consume front grip and reduce steering response.

## Weight Transfer And Aero

Static axle load comes from mass, effective gravity, and
`body.front_weight_fraction`.
Longitudinal load transfer changes front/rear normal load:

```text
transfer = mass * longitudinal_acceleration * center_of_mass_height / wheelbase
```

Positive acceleration moves load rearward. Braking moves load forward.

Lateral load transfer changes left/right normal load:

```text
transfer = mass * lateral_acceleration * center_of_mass_height / track_width
```

`tires.front_roll_stiffness_fraction` controls how much of that transfer lands
on the front pair versus the rear pair. Tire grip uses degressive load
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

## Banking

Track banking now contributes to the planar physics model. `RaceSession`
passes the sampled banking angle into `Vehicle::step`.

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

Bank angle now uses quintic smoothstep easements centered on turn entry and
exit. With the default `geometry.bank_transition_runout_m = 50.0`, banking
starts blending in 50 m before turn entry, reaches full banking 50 m into the
corner, then blends out symmetrically at corner exit. This gives continuous
height/roll acceleration and avoids abrupt entrance/exit humps.

## Track And Surfaces

The procedural stadium oval is approximately 2.5 miles around its centerline.
The road includes config-driven quintic banking transitions up to 9.2 degrees.
The track surface remains an analytic smooth road plane, but banking
contributes lateral gravity, effective-normal-load adjustments, and renderer
road height/bank orientation.

The track classifies the car as asphalt, apron, or grass. Each surface applies
config-driven lateral-grip and rolling-resistance multipliers. Grass also has
a separate longitudinal grip multiplier: it keeps low cornering grip so the car
slides easily, but has enough straight-line tire bite to escape with digital
throttle. `RaceSession` then applies extra grass-trap rolling resistance plus
speed-sensitive damping so fast off-track excursions bleed speed aggressively
without stranding the car at low speed. Leaving asphalt invalidates the current
timed lap. The
outer wall and visible inner infield wall use yaw-aware four-corner
bounding-box collision, contact-point linear/yaw impulse response, and low
restitution.

Lap timing starts after crossing start/finish and requires four ordered
checkpoints, preventing shortcut laps.

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

- Suspension runs against a smooth local track plane; there are no per-wheel
  terrain height samples, potholes, curbs, or arbitrary 3D mesh contacts yet.
- The tire model has relaxation length, dynamic slip ratio, combined-slip
  limiting, static setup camber thrust, and a lightweight slip/usage-driven tire
  temperature and thermal grip state, but no pressure, wear, carcass modes,
  dynamic camber gain, or contact-patch deformation model.
- Brake discs are currently clean metallic render parts, not thermal brake
  simulations.
- Tire smoke/dust and undertray sparks are renderer-side presentation effects,
  not particle/fluid/debris simulations. Exhaust flame and rear-exhaust heat
  shimmer are intentionally not rendered.
- No general collision shapes; only the oval outer and inner wall loops are
  active, using a yaw-aware vehicle bounding box
- The engine curve is intentionally simple and not based on measured engine data

These are intentional current-model limits. The vehicle already
produces momentum, understeer/oversteer balance, per-wheel load transfer,
sprung chassis attitude, combined-slip behavior, brake-bias effects,
ride-height-sensitive aero load, drag, gearing, wheelspin/lockup telemetry,
lateral slip, and braking behavior while keeping the vehicle module
replaceable.
