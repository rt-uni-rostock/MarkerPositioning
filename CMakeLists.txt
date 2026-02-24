# CMakeList.txt: CMake-Projekt für "MarkerPositioning". Schließen Sie die Quelle ein, und definieren Sie
# projektspezifische Logik hier.
#
cmake_minimum_required (VERSION 3.16)

# Aktivieren Sie Hot Reload für MSVC-Compiler, sofern unterstützt.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MarkerPositioning" LANGUAGES C CXX)

# MSVC soll alle Dateien als UTF-8 interpretieren
add_compile_options(/utf-8)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: binde lokale Bibliotheken ein
if (WIN32)
	set(OpenCV_DIR "C:/Code/opencv_build2/build/install/x64/vc17/lib/")
endif()

find_package(OpenCV REQUIRED)

# load configuration file and validate settings
if(EXISTS "${CMAKE_SOURCE_DIR}/config.cmake")
	include(${CMAKE_SOURCE_DIR}/config.cmake)
else()
	message(STATUS "config.cmake not found - using defaults")
endif()

message(STATUS "Video Stream: ${VIDEO_STREAM}")
message(STATUS "Tag Type:     ${TAG_TYPE}")

set(VALID_VIDEO_STREAMS RTSP RTP LUCID)
set(VALID_TAG_TYPES APRILTAG ARUCO)

list(FIND VALID_VIDEO_STREAMS ${VIDEO_STREAM} VIDEO_STREAM_INDEX)
if (VIDEO_STREAM_INDEX EQUAL -1)
	message(FATAL_ERROR "Invalid VIDEO_STREAM: ${VIDEO_STREAM}. Allowed values are: ${VALID_VIDEO_STREAMS}")
endif()

list(FIND VALID_TAG_TYPES ${TAG_TYPE} TAG_TYPE_INDEX)
if (TAG_TYPE_INDEX EQUAL -1)
	message(FATAL_ERROR "Invalid TAG_TYPE: ${TAG_TYPE}. Allowed values are: ${VALID_TAG_TYPES}")
endif()

# add subdirectories
# add_subdirectory(MainThreadsManager)
add_subdirectory(DetectionPipeline)
add_subdirectory(IO)
add_subdirectory(Helper)
add_subdirectory(SettingsReader)
add_subdirectory(Supervisor)
add_subdirectory(external/sqlite)
add_subdirectory(external/spdlog)

# Fügen Sie der ausführbaren Datei dieses Projekts eine Quelle hinzu.
add_executable (MarkerPositioning 
	MarkerPositioning.cpp 
	MarkerPositioning.h 
	Helper/Logger.h
	Helper/Logger.cpp)

# todo apriltag / aruco module inclusion

target_link_libraries(MarkerPositioning PRIVATE 
#	MainThreadsManager
	Supervisor
	$<$<CONFIG:Debug>:${OpenCV_LIBS_DEBUG}>
    $<$<CONFIG:Release>:${OpenCV_LIBS}>
)

target_include_directories(MarkerPositioning 
	PRIVATE 
		${OpenCV_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_compile_definitions(MarkerPositioning
    PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)

# Debug SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
# Release SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN
# Realtime SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF

# TODO: Fügen Sie bei Bedarf Tests hinzu, und installieren Sie Ziele.
