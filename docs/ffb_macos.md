# Force Feedback On macOS

## Current Status

Force feedback now has an SDL3 haptic backend behind `ForceFeedbackManager`.
At startup, the app loads `config/ffb_default.json`, waits for the configured
wheel selected by `config/input_default.json`, and tries to open haptics from
that joystick with `SDL_OpenHapticFromJoystick`.

The backend attempts effects in this order:

1. `SDL_HAPTIC_CONSTANT` for physics-driven signed constant-force streaming
2. `SDL_HAPTIC_SPRING` as a centering-condition fallback
3. `SDL_HAPTIC_LEFTRIGHT` as a rumble fallback
4. SDL simple rumble as a final fallback
5. clean no-op status if no usable haptic path is exposed

The overlay reports the active FFB status, for example constant force active,
spring fallback, rumble fallback, no selected haptic device, or unsupported
effects.

The Logitech Windows Steering Wheel SDK and DirectInput are not used or assumed
to exist on macOS. **A real G29 has not yet been tested on this macOS path, so
the project does not yet claim confirmed G29 force feedback.** If SDL marks the
device as haptic but exposes no usable wheel force effects, the backend will
fall back or disable itself gracefully.

## Physics Inputs

The requested force is normalized, smoothed, clamped, and generated from:

- front tire lateral force multiplied by pneumatic and mechanical trail for
  self-aligning torque
- pneumatic-trail falloff and front slip-angle understeer lightening past the
  peak slip region
- low-speed steering-angle centering
- steering-angle velocity damping
- apron/grass vibration overlays
- wall-contact jolt envelopes

The coefficients live in `config/ffb_default.json`:

- `enabled`
- `master_strength`
- `centering_strength`
- `damping`
- `aligning_torque_scale`
- `pneumatic_trail_m`
- `mechanical_trail_m`
- `peak_slip_angle_deg`
- `understeer_lightening`
- `grass_vibration`
- `collision_jolt`
- `max_force`

## Remaining Investigation

1. Connect a real G29 on macOS and record the overlay-reported backend status.
2. Verify whether SDL exposes constant-force, spring, or only rumble/no haptic
   effects for that wheel.
3. Tune force sign and strength on hardware at low speed before driving fast.
4. Investigate Apple Game Controller haptics and IOKit HID only if SDL does not
   expose useful wheel effects.
