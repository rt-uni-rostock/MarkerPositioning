#=========================================================
# Build configuration file for MarkerPositioning
# This file is included by the main CMakeLists.txt
#=========================================================


# ---- Video Stream Selection ----
# Allowed values: "RTSP", "RTP", "LUCID"
set(VIDEO_STREAM  "RTP")

# ---- Tag Detection Type ----
# Allowed values: "ARUCO", "APRILTAG"
set(TAG_TYPE  "APRILTAG")