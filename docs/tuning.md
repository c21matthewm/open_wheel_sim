# Tuning

Vehicle settings live in `config/vehicle_openwheel_default.json`. Restart the
app after editing; live reload is not implemented yet.

## Steering

- `steering.max_road_wheel_angle_deg`: maximum physical front-wheel angle. The
  default `18.0` is the full rack limit and remains available at every speed.
- `steering.high_speed_steer_scale`: input-side keyboard response scale once
  the threshold speed is reached. It slows how quickly keyboard steering moves
  toward full lock, but it does not clamp the physical rack angle.
- `steering.steer_speed_threshold_mps`: speed where the high-speed steering
  scale is fully applied.
- `keyboard.steer_rate`: how quickly keyboard steering reaches full input.
- `keyboard.return_rate`: how quickly keyboard steering recenters.

Escape opens a runtime tuning menu where keyboard steer/return rates, brake
bias, automatic shifting, and records/ghost state can be adjusted for the
current run. These runtime changes are not written back to JSON yet.

## Grip And Balance

- `tires.friction_coefficient`: peak axle-force multiplier.
- `tires.front_cornering_stiffness_n_per_rad`: front response to slip angle.
- `tires.rear_cornering_stiffness_n_per_rad`: rear response to slip angle.
- `tires.longitudinal_stiffness_n`: drive/brake response to slip ratio.
- `tires.camber_stiffness_n_per_rad`: camber-thrust contribution before the
  combined-slip limiter.
- `tires.stiffness_speed_softening`: high-speed cornering-stiffness reduction
  from tire carcass growth. Higher values make the car lazier at very high
  wheel speeds.
- `tires.stiffness_speed_ref_rps`: wheel angular speed where the softening
  coefficient reaches its reference effect.
- `tires.lateral_peak_slip_angle_deg`: approximate lateral slip angle where
  the Pacejka-lite curve reaches peak grip.
- `tires.longitudinal_peak_slip_ratio`: approximate drive/brake slip ratio
  where longitudinal grip peaks.
- `tires.longitudinal_grip_fraction`: longitudinal peak grip relative to
  lateral peak grip inside the combined-slip friction ellipse. Lower values
  make braking/throttle consume the longitudinal budget sooner while preserving
  more lateral authority.
- `tires.curve_shape_factor`, `curve_curvature_factor`, `post_peak_falloff`:
  compact tire-curve shape controls for peak sharpness and post-peak falloff.
  Higher `post_peak_falloff` makes over-driving past peak slip shed lateral
  force more aggressively, so spin-outs come from the tire model instead of
  steering clamps.
- `tires.pacejka_min_stiffness_factor`, `pacejka_peak_force_target`, and
  `pacejka_max_stiffness_factor`: safety guard for under-stiff tire configs.
  If stiffness is too low for the curve to approach the friction limit near
  peak slip, the internal Pacejka factor is boosted up to the configured max.
- `tires.thermal_optimal_c`: tire temperature where thermal grip peaks.
- `tires.thermal_window_c`: Gaussian grip-window width around the optimal
  temperature. Narrower values make warm-up and overheating more punishing.
- `tires.thermal_grip_min`: minimum thermal grip multiplier for very cold or
  overheated tires.
- `tires.lateral_load_transfer_grip_loss`: small global grip trim as lateral
  load transfer rises.
- `tires.load_sensitivity`: heavily loaded tires become slightly less efficient.
- `tires.front_roll_stiffness_fraction`: legacy fallback share of lateral load
  transfer handled by the front tires when the suspension ARB roll-stiffness
  fields are absent.
- `body.front_weight_fraction`: static front load share.
- `body.center_of_mass_height_m`: higher values increase braking and throttle
  load transfer and left/right transfer in corners.
- `body.track_width_m`: wider values reduce the current lateral load-transfer
  effect.
- `suspension.front_arb_nm_per_rad` and `rear_arb_nm_per_rad`: anti-roll-bar
  contribution to roll-axis load-transfer distribution. More front ARB shifts
  load transfer forward and increases understeer; more rear ARB shifts it
  rearward and makes the car more willing to rotate.

More front stiffness generally sharpens turn-in. Excess rear stiffness relative
to available rear grip can make the car less forgiving.

## Acceleration And Braking

- `powertrain.engine_torque_nm`: base engine torque before the simple RPM curve.
- `powertrain.torque_curve_knots`: up to eight `[rpm_norm, torque_norm]`
  pairs for the piecewise-linear engine torque multiplier. RPM is normalized
  by redline. The default is soft below the powerband, strongest in the
  upper-mid/top range, and cuts to zero at redline.
- `powertrain.engine_braking_n`: off-throttle rear-axle drag from the engine.
- `powertrain.drivetrain_efficiency`: torque loss through the drivetrain.
- `powertrain.drive_front_fraction`: drive sent to the front tires. The default
  open-wheel tune keeps this at `0.0`.
- `powertrain.differential_load_bias`: how much drive follows left/right normal
  load instead of a fixed 50/50 split.
- `powertrain.wheel_radius_m`: driven tire radius used for force and RPM.
- `powertrain.final_drive_ratio`: final drive multiplier.
- `powertrain.gear_1` through `powertrain.gear_6`: scalar gear ratios.
  The default IMS speedway stack redlines 6th at roughly 239 mph with the
  configured wheel radius and keeps 4th-6th tightly spaced.
- `powertrain.idle_rpm`, `redline_rpm`, `shift_up_rpm`, `shift_down_rpm`:
  drivetrain telemetry and automatic shift points.
- `powertrain.limiter_start_margin_rpm` and `limiter_full_margin_rpm`: how
  close to redline the RPM limiter starts and reaches full cut. The defaults
  keep top-gear speedway pulls from losing power before the shift/redline zone.
- `powertrain.automatic_transmission`: keeps keyboard driving usable without a
  shifter when enabled. When disabled, paddles/keyboard shift inputs are the
  only way to change gear.
- `powertrain.lsd_preload_nm`: rear limited-slip differential preload that
  resists left/right rear wheel-speed difference even at low throttle.
- `powertrain.lsd_ramp_factor`: additional LSD locking torque as rear drive
  torque rises.
- `powertrain.lsd_sensitivity`: maps rear wheel-speed difference into locking
  torque before clamping to the available preload/ramp lock.
- `brakes.max_brake_force_n`: total brake force demand before tire limiting.
- `brakes.brake_bias`: front brake share. Higher values make braking more
  front-biased.
- `brakes.brake_gamma`: nonlinear brake input response.
- `aero.drag_n_per_mps2`: quadratic speed resistance. The speedway default is
  a low-drag oval value tuned with the IMS gear stack for a 235-240 mph pull.
- `aero.downforce_n_per_mps2`: quadratic vertical load added with speed.
- `aero.front_downforce_fraction`: front share of aero load.
- `aero.ride_height_balance_sensitivity`: CoP shift from front/rear tunnel
  strength as the aero platform compresses.
- `aero.brake_cop_shift`: forward CoP migration under braking.
- `aero.stall_cop_shift`: CoP migration when the front or rear floor bottoms.
- `aero.instant_load_fraction`: small current-platform aero load applied
  directly to tire normal loads for sharper high-speed aero response.
- `aero.yaw_damping_nm_per_rad_s`: speed-squared yaw damping coefficient.
  Higher values make the car settle yaw perturbations more strongly at
  speedway speeds; the current baseline is deliberately low enough that it
  does not mask tire breakaway.
- `aero.yaw_damping_reference_speed_mps`: speed where the yaw damping
  coefficient is applied at full scalar strength before the `speed^2` scale.
- `aero.yaw_damping_rear_slide_min_scale` and
  `aero.yaw_damping_rear_slide_full_saturation`: fade yaw damping during rear
  tire over-limit slides so excess steering/throttle can produce organic spins.
- `resistance.rolling_resistance_n`: constant rolling resistance while moving.

Throttle, brake, and lateral tire forces now share each wheel's friction budget.
If one of the four tire usage values is near `1.00` in the overlay, that contact
patch is saturated. At that point adding brake, throttle, or steering will take
grip away from the other force directions at that tire.

## Input Mapping

Use the `F2` diagnostic display, then edit `config/input_default.json`:

- `wheel.name_contains`: substring used to select a device
- `steer_axis`, `throttle_axis`, `brake_axis`: raw SDL axis indices
- `shift_up_button`, `shift_down_button`: raw SDL button indices
- `throttle_inverted`, `brake_inverted`: for G29-style pedals that report
  released near `+1.0` and fully pressed near `-1.0`
- `steer_deadzone`, `pedal_deadzone`: remove small unwanted raw-axis movement
- `steer_gamma`, `throttle_gamma`, `brake_gamma`: nonlinear input response
  after deadzone/inversion mapping. Values above `1.0` soften initial response;
  values below `1.0` make early input more aggressive.

Raw axis values shown by SDL range from `-1.000` to `+1.000`.

## Force Feedback

Force-feedback settings live in `config/ffb_default.json`. The SDL3 haptic
backend tries constant force first, then spring and rumble fallbacks. Real G29
support still depends on what macOS and SDL expose for the connected wheel.

- `enabled`: enables the SDL haptic backend
- `master_strength`: final gain applied before clipping
- `centering_strength`: low-speed steering-angle centering
- `damping`: resistance against rapid steering-angle changes
- `aligning_torque_scale`: gain for front-tire trail self-aligning torque
- `pneumatic_trail_m` and `mechanical_trail_m`: trail lengths used to convert
  front tire lateral forces into steering-column torque
- `peak_slip_angle_deg`: FFB slip angle where pneumatic trail and steering
  effort start dropping quickly
- `understeer_lightening`: reduces SAT when front slip exceeds the peak region
- `grass_vibration`: vibration overlay on grass and mild apron rumble
- `collision_jolt`: short wall-contact shock envelope
- `max_force`: final normalized force clamp

## Track And Surface Tuning

Track settings live in `config/track_oval_default.json`:

- `geometry.straight_length_m` and `corner_radius_m`: determine lap shape and
  centerline length
- `geometry.road_width_m` and `apron_width_m`: surface-zone widths
- `geometry.outer_wall_gap_m` and `inner_wall_gap_m`: wall positions outside
  the racing surface; the inner gap creates the visible infield grass runout
- `geometry.banking_degrees`: maximum rendered corner banking
- `geometry.bank_transition_runout_m`: easement distance before/after turn
  entry and exit; the default 50 m uses quintic smoothing to avoid banking
  humps
- `geometry.bank_transition_fraction`: retained legacy value; the current
  easement model is controlled by `bank_transition_runout_m`
- `surfaces.*_grip_multiplier`: scales available lateral tire force
- `surfaces.grass_longitudinal_grip_multiplier`: scales grass drive/brake tire
  force separately from lateral grip so the car can accelerate out of grass
  without giving the grass asphalt-like cornering grip
- `surfaces.*_resistance_multiplier`: scales rolling resistance
- `barrier.restitution`: controls how much outward wall velocity rebounds
- `render.segments`: procedural oval mesh resolution

Leaving asphalt invalidates the current timed lap.
