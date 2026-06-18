# macOS Setup

## Install The Toolchain

VS Code plus the integrated Terminal is the primary workflow. Full Xcode is
optional. For normal VS Code/command-line builds, install or repair Xcode
Command Line Tools:

```sh
xcode-select --install
xcode-select -p
xcrun --sdk macosx --show-sdk-path
```

Install Homebrew from <https://brew.sh/> if it is not already available, then:

```sh
brew install cmake sdl3
```

Check the tools:

```sh
cmake --version
clang++ --version
brew list --versions sdl3
```

If the selected SDK contains libc++ but Apple Clang's default include path is
missing, CMake prints a warning and uses the SDK copy. If `clang++` still cannot
find standard C++ headers, reinstall Command Line Tools or install Xcode and
select it:

```sh
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
```

## Optional: Enable Full Xcode Project Generation

Command Line Tools provide Apple Clang, SDK access, and terminal build tools,
but they do not contain the full Xcode IDE required by CMake's `Xcode`
generator.

To enable full Xcode project generation:

1. Install Xcode from the Mac App Store or Apple's developer downloads site.
2. Launch Xcode once and allow it to install any requested components.
3. Select the full Xcode developer directory:

```sh
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -runFirstLaunch
```

Verify that the selected toolchain is full Xcode:

```sh
xcode-select -p
xcodebuild -version
```

`xcode-select -p` should print:

```text
/Applications/Xcode.app/Contents/Developer
```

Use a fresh generation directory after switching toolchains:

```sh
cmake -S . -B build-xcode -G Xcode \
  -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
cmake --build build-xcode --config Release
open build-xcode/LightweightOpenWheelSim.xcodeproj
```

In Xcode, select the `LightweightSim` scheme and `My Mac`, then press Command+R.
Keep `CMakeLists.txt` as the source of truth; regenerate the Xcode project after
changing build structure.

To switch back to the standalone Command Line Tools later:

```sh
sudo xcode-select --switch /Library/Developer/CommandLineTools
```

## Configure And Build

Single-config Makefile or Ninja build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Release build:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Xcode project:

```sh
cmake -S . -B build-xcode -G Xcode
cmake --build build-xcode --config Release
```

If CMake cannot find SDL3 installed by Homebrew, pass its prefix explicitly:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
```

## Run

The quickest build-and-launch command is:

```sh
./scripts/build_and_run.sh
```

From Terminal:

```sh
./build/LightweightSim.app/Contents/MacOS/LightweightSim
```

From Finder or Launch Services:

```sh
open build/LightweightSim.app
```

The build copies `config`, `assets`, and a README into the app bundle. During
development, a terminal launch from the repository root reads the source
`config` directory first.

See `docs/running.md` for the complete run workflow and expected controls.

## TV And Wheel

Connect the Mac to the TV with HDMI or USB-C-to-HDMI, select a 60 Hz mode in
System Settings > Displays, and enable the TV's low-latency game mode. Avoid
AirPlay. Connect the G29 directly by USB where practical.

Use `F2` to inspect raw SDL3 device values. The current build does not claim
working G29 force feedback on macOS.
