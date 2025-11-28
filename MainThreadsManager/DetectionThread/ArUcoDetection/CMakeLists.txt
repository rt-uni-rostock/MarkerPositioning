cmake_minimum_required (VERSION 3.16)

add_library (ArUcoDetection
	"ArUcoDetection.h"
	"ArUcoDetection.cpp"
)

target_compile_features(RTSPStream PUBLIC cxx_std_20)