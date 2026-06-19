# Tuning

Vehicle settings live in `config/vehicle_openwheel_default.json`. Restart the
app after editing; live reload is not implemented yet.

## Steering

- `steering.max_road_wheel_angle_deg`: maximum physical front-wheel angle. The
  default `18.0` is the full rack limit. Direct wheel input can still request
  the full rack range at every speed.
- `steering.high_speed_steer_scale`: input-side keyboard response scale once
  the threshold speed is reached. It slows how quickly keyboard steering moves
  and caps the maximum keyboard target at high speed so held digital steering
  cannot command full lock and stall the front tires on speedway straights.
- `steering.steer_speed_threshold_mps`: speed where the high-speed steering
  scale is fully applied.
- `keyboard.steer_rate`: how quickly keyboard steering reaches full input.
- `keyboard.return_rate`: how quickly keyboard steering recenters.

Escape opens a runtime tuning menu where keyboard steer/return rates, front
wing, rear wing, brake bias, engine map, automatic shifting, aero preset, and
records/ghost state can be adjusted for the current run. These runtime changes
are not written back to JSON yet. Press `M` while driving to cycle the engine
map without opening the menu.

## Grip And Balance

- `tires.friction_coefficient`: peak axle-force multiplier.
- `tires.front_cornering_stiffness_n_per_rad`: front response to slip angle.
- `tires.rear_cornering_stiffness_n_per_rad`: rear response to slip angle.
- `tires.longitudinal_stiffness_n`: drive/brake response to slip ratio.
- `tires.camber_stiffness_n_per_rad`: camber-thrust contribution before the
  combined-slip limiter.
- `tires.camber_angle_front_rad` and `camber_angle_rear_rad`: static physical
  setup camber for the active speedway package. The road-course override fields
  use the same units.
- `suspension.front_camber_gain_deg_per_m` and
  `rear_camber_gain_deg_per_m`: suspension kinematic camber gain. Positive
  values add negative camber in bump and remove it in droop. These interact
  with spring and ARB tuning: a stiffer axle compresses less per g, so the tire
  reaches less dynamic negative camber at peak load than with a softer setup.
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
- `tires.wear_min_grip`: grip multiplier approached by a fully worn tire.
- `tires.wear_sliding_rate_per_work`: tire-life loss from sustained lateral
  slip and near-limit usage.
- `tires.wear_wheelspin_rate_per_work`: tire-life loss from drive/brake slip
  and wheelspin.
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
- `suspension.front_damper_bump_n_per_mps` and
  `rear_damper_bump_n_per_mps`: compression damping. More low/shaft-speed bump
  support slows roll and weight-transfer rate; too much can make turn-in and
  banking transitions nervous.
- `suspension.front_damper_rebound_n_per_mps` and
  `rear_damper_rebound_n_per_mps`: extension damping. More rebound slows wheel
  return after compression and can keep a tire loaded longer, but too much can
  jack the platform down and reduce contact recovery. Increasing front rebound
  relative to front bump can reduce mid-corner understeer by helping the front
  stay loaded through the apex.
- `suspension.front_damper_n_per_mps` and `rear_damper_n_per_mps`: legacy
  fallback values used only when the split bump/rebound fields are absent.
- `suspension.longitudinal_load_transfer_tau_s`: first-order response time for
  acceleration/braking load transfer. Higher values make front/rear tire load
  build more gradually; lower values make brake-lockup and launch response more
  immediate. The default is `0.08` seconds.

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
- `powertrain.fuel_capacity_gallons` and `fuel_initial_gallons`: maximum and
  starting fuel load for the current run.
- `powertrain.fuel_burn_gal_per_s_at_redline`: full-load standard-map fuel
  burn reference at redline.
- `powertrain.fuel_average_burn_response_hz`: smoothing rate for the average
  burn value used by the HUD laps-remaining estimate.
- `powertrain.fuel_idle_load_factor` and `fuel_idle_rpm_factor`: idle-side
  shape factors for fuel burn at low throttle and low RPM.
- `powertrain.engine_map_*_fuel_multiplier`: Lean/Standard/Rich mixture fuel
  burn scalars.
- `powertrain.engine_map_*_torque_multiplier`: Lean/Standard/Rich engine
  output scalars.
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
- `aero.yaw_damping_rear_slide_min_scale`: minimum aero yaw-damping floor after
  heading alignment is applied. Forward motion keeps full damping, while a
  sideways spin collapses toward this floor instead of being caught by full
  speed-squared damping.
- `aero.yaw_damping_rear_slide_full_saturation`: legacy config field retained
  for compatibility with older setup files; the current solver uses geometric
  heading alignment rather than rear-slip saturation to fade aero yaw damping.
- `setup.front_wing_setting`: runtime/default front wing offset. More front
  wing shifts aero balance forward and increases effective front cornering
  stiffness.
- `setup.rear_wing_setting`: runtime/default rear wing offset. More rear wing
  increases total downforce, rear tire authority, and drag.
- `setup.wing_setting_min` and `wing_setting_max`: clamp range for live wing
  adjustments.
- `setup.front_wing_aero_balance_per_step`,
  `front_wing_cornering_stiffness_per_step`,
  `rear_wing_downforce_per_step`, `rear_wing_drag_per_step`, and
  `rear_wing_cornering_stiffness_per_step`: per-click physics effects for the
  setup menu wing controls.
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

Track settings live in `config/track_oval_default.json` and
`config/track_hillcircuit_default.json`. The runtime selector is
`simulation.active_track` in `config/graphics_default.json`; it defaults to
`oval` and accepts `hillcircuit`. `SIM_ACTIVE_TRACK=hillcircuit` can override
the selector for one launch.

- `track.type`: selects the concrete track implementation. The current
  speedway value is `oval`; the factory also accepts `speedway` as an alias.
- `track_type`: selects the Hill Circuit implementation in the road-course
  config.
- `geometry.straight_length_m` and `corner_radius_m`: determine lap shape and
  centerline length
- `geometry.road_width_m` and `apron_width_m`: physics surface-zone widths.
  The default oval uses a 16.764 m average IMS racing surface and a 6.096 m
  apron.
- `geometry.straight_road_width_m` and `geometry.turn_road_width_m`: render-only
  oval visual road widths. The default IMS values are 15.240 m straights and
  18.288 m turns; surface classification still uses `road_width_m`.
- `geometry.outer_wall_gap_m` and `inner_wall_gap_m`: wall positions outside
  the racing surface; the inner gap creates the visible infield grass runout
- `geometry.banking_degrees`: maximum rendered corner banking
- `geometry.bank_transition_runout_m`: easement distance before/after turn
  entry and exit; the default IMS 91.44 m uses quintic smoothing to spread the
  banking transition over roughly 300 ft
- `geometry.bank_transition_fraction`: retained legacy value and set to 0.09
  for IMS consistency; the current easement model is controlled by
  `bank_transition_runout_m`
- `geometry.pit_lane_*`, `geometry.pit_box_*`, `geometry.pit_wall_height_m`,
  and `geometry.yard_of_bricks_*`: render-only oval frontstretch settings for
  pit lane asphalt, pit wall, pit box markings, garage massing, and the brick
  start/finish strip
- `checkpoints_m`: ordered oval checkpoint marker distances; the first value is
  the start/finish timing origin. The default set is
  `[304.8, 1310.64, 2316.48, 3322.32]`. Oval distance zero is the center of
  the front straight, so the first gate is 304.8 m forward from its center.
- `surfaces.*_grip_multiplier`: scales available lateral tire force. The
  simulator samples surface type per wheel contact patch, so a car straddling
  the apron/asphalt boundary gets asymmetric grip instead of one car-wide
  multiplier.
- `surfaces.apron_grip_multiplier` and `surfaces.apron_resistance_multiplier`:
  tune the sealed IMS apron as a usable alternate line; defaults are 0.92 grip
  and 1.25 rolling resistance
- `surfaces.grass_longitudinal_grip_multiplier`: scales grass drive/brake tire
  force separately from lateral grip so the car can accelerate out of grass
  without giving the grass asphalt-like cornering grip
- `surfaces.*_resistance_multiplier`: scales per-wheel rolling resistance.
  Grass defaults to high but escapable resistance, with extra speed-sensitive
  grass-trap damping in `RaceSession` reduced so low-speed throttle can pull
  the car back to the racing surface.
- `terrain.terrain_height_enabled`: enables deterministic per-wheel contact
  height, road normal, roughness, and curb-height sampling. Disable this to
  return vehicle physics to the old smooth local road plane while leaving
  surface grip/resistance classification unchanged.
- `terrain.asphalt_roughness_m`: clean-asphalt procedural micro height.
  Defaults are intentionally subtle, in the 3-8 mm range.
- `terrain.oval_seam_amp_m`: oval asphalt/apron and edge seam bump amplitude.
  Higher values make the inside-edge transition and visual pavement seams more
  active through the tire vertical spring.
- `terrain.oval_undulation_amp_m`: low-frequency oval asphalt waviness over
  long wavelengths. Keep this small to avoid artificial chassis shake.
- `terrain.curb_height_m`: peak procedural curb height on Hill Circuit curb
  strips. The profile is filtered and rumble-shaped by distance along the
  track.
- `terrain.contact_height_filter_tau_s`: first-order contact-height smoothing
  inside `Vehicle::step`. Smaller values make seams/curbs sharper; larger
  values reduce buzz and protect the 360 Hz suspension integration.
- `barrier.restitution`: controls how much outward wall velocity rebounds
- `render.segments`: procedural oval mesh resolution

Hill Circuit-specific fields:

- `lap_length_m`, `spawn_distance_m`, and `checkpoints`: define road-course lap
  timing and spawn state.
- `elevation_spine`: distance/elevation control points for the cubic height
  profile; the track derives road pitch from this spline.
- `curb_width_m` and `runoff_width_m`: define curb and grass bands outside the
  12 m asphalt road.
- `surface_curb.*`: curb lateral grip, longitudinal grip, and rumble-like
  rolling resistance. Curbs do not invalidate laps.
- `surface_grass.*`: road-course grass grip and resistance values.

Leaving asphalt invalidates the current timed lap except for curb use.
