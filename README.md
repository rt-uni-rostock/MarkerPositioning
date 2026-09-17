# MarkerPositioning

This software library provides visual localization of fiducial markers in images and video streams. 

# Features

Currently, the library supports following input stream types:

* RTSP video streams
* RTP video streams
* LUCID Vision GigE video streams

Regarding the visual localization of fiducial markers, the library supports:

* AprilTags
* (ArUco markers) - coming soon

The library outputs the 6-DoF pose of each detected marker relative to the camera frame.

# Installation

## Windows

1. Clone the repository:
   ```bash
   git clone https://github.com/rt-uni-rostock/MarkerPositioning.git
   ```
2. Open `MarkerPositioning.sln` in Visual Studio 2022
3. Build the solution (preset: `x64-release` recommended)
4. Run the binary from `/out/build/x64-release/MarkerPositioning.exe`

## Linux (Debian 13 / Raspberry Pi 5)

**Quick Start:**
```bash
cd ~/MarkerPositioning/MarkerPositioning
tools/install-linux.sh              # Vollständige automatisierte Installation
# oder manuell:
tools/diagnose-linux.sh             # Überprüfe Abhängigkeiten
./build.sh --preset linux-debian    # Debian x86_64 mit LUCID
./build.sh --preset linux-rpi5      # Raspberry Pi 5 ohne LUCID
```

For detailed instructions, see:
- **Quick Start:** `docs/QUICKSTART.md`
- **Full Setup:** `docs/linux-setup.md`
- **Implementation Details:** `docs/LINUX_IMPLEMENTATION_CHECKLIST.md`

# Usage

For configuration of this library, there is a build-time option and a runtime configuration file.

1. `ENABLE_LUCID` (CMake option, default `ON`): controls whether support for LUCID Vision GigE cameras (Arena SDK) is compiled in. Some target systems (e.g. Raspberry Pi) don't need a LUCID camera and should not require an Arena SDK installation; configure with `-DENABLE_LUCID=OFF` (or use a matching CMake preset, see `CMakePresets.json`) to build without it. All other stream types (RTSP, RTP, USB webcam) and the marker type (AprilTags, ArUco) are always compiled in and selected at runtime via `Settings.json`.
1. `settings/Settings.json`: Here you can configure the remaining parameters, such as camera intrinsics, marker size, and other relevant settings. Refer to the provided `settings/Settings_template.json` for guidance. Remember, that json files do not support comments! This master copy is picked up by `build.sh` and copied next to the built executable as `Settings.json`, which is where the application reads it from at runtime.

See `docs/linux-setup.md` for detailed instructions on setting up the required dependencies (OpenCV, AprilTag, and optionally the LUCID Arena SDK) on Debian Linux and Raspberry Pi 5.

# TODO
* Add support for ArUco markers
* describe the software in more detail