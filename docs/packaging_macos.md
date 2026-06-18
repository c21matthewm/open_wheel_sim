# macOS Packaging

## App Bundle

CMake builds `LightweightSim.app`, bundles SDL3 under `Contents/Frameworks`,
copies `config`, `assets`, and `README.txt` into `Contents/Resources`, then
applies an ad-hoc signature for local testing.

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Re-run an ad-hoc signature after making any manual bundle changes:

```sh
codesign --force --deep --sign - build-release/LightweightSim.app
codesign --verify --deep --strict build-release/LightweightSim.app
```

## Zip Package

```sh
ditto -c -k --sequesterRsrc --keepParent \
  build-release/LightweightSim.app LightweightSim-macOS.zip
```

Recipients may need to approve an unsigned or ad-hoc-signed local build in
System Settings > Privacy & Security. Notarization and Developer ID signing are
separate future distribution steps.

## External TV

Connect by HDMI or USB-C-to-HDMI, select the TV and 60 Hz output in System
Settings > Displays, enable its low-latency game mode, then use `F11` or
Command+F for fullscreen.
