cmake_minimum_required (VERSION 3.16)

add_library (ArUcoDetection STATIC
	ArUcoDetection.h
	ArUcoDetection.cpp
)

target_compile_features(ArUcoDetection PUBLIC cxx_std_20)

target_include_directories(ArUcoDetection PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ArUcoDetection PRIVATE 
	Helper
	SettingsReader
	Detection
)

target_compile_definitions(ArUcoDetection
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)