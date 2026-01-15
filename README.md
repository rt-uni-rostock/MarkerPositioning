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

To install the MarkerPositioning library, follow these steps:
1. Clone the repository:
   ```bash
   git clone https://github.com/rt-uni-rostock/MarkerPositioning.git```
2. a) Linux system: run the installation script build.sh
2. b) Windows system: under /out/build/release run MarkerPositioning.exe

# Usage

For configuration of this library, there are two configuration files.

1. config.cmake: Here you can configure the input stream type (RTSP, RTP, LUCID Vision GigE) and the marker type (AprilTags, ArUco markers).
1. Settings.json: Here you can configure the remaining parameters, such as camera intrinsics, marker size, and other relevant settings. Refer to the provided Settings_template.json for guidance. Remember, that json files do not support comments!

# TODO
* Add support for ArUco markers
* describe the software in more detail