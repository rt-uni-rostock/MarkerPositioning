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
if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows: binde lokale Bibliotheken ein

# =============================================================
# External Dependencies
# =============================================================

add_subdirectory(external/spdlog)
add_subdirectory(external/sqlite)

# =============================================================
# OpenCV
# =============================================================

if (WIN32)
	set(OpenCV_DIR "C:/Code/opencv_build2/build/install/x64/vc17/lib/")
endif()

find_package(OpenCV REQUIRED)

# =============================================================
# AprilTag
# =============================================================

if(WIN32)
    set(APRILTAG_INCLUDE_DIR
        "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag"
    )

    set(APRILTAG_LIB
        "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag/build/Release/apriltag.lib"
    )
else()
    find_path(APRILTAG_INCLUDE_DIR apriltag.h PATHS /usr/local/include/apriltag/)
    find_library(APRILTAG_LIB apriltag PATHS /usr/local/lib/)

    if(NOT APRILTAG_INCLUDE_DIR OR NOT APRILTAG_LIB)
        message(FATAL_ERROR "Could not find AprilTag library")
    endif()
endif()

add_library(AprilTag STATIC IMPORTED)

set_target_properties(AprilTag PROPERTIES
    IMPORTED_LOCATION ${APRILTAG_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${APRILTAG_INCLUDE_DIR}
)

# =============================================================
# Configuration file (OLD)
# =============================================================

if(EXISTS "${CMAKE_SOURCE_DIR}/config.cmake")
	include(${CMAKE_SOURCE_DIR}/config.cmake)
else()
	message(STATUS "config.cmake not found - using defaults")
endif()


# =============================================================
# Helper
# ==================================    ===========================

add_library(Helper
	Helper/Logger.cpp
)

target_include_directories(Helper PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Helper
)

target_link_libraries(Helper PUBLIC
    spdlog::spdlog
)

# =============================================================
# Global Logging Level
# =============================================================

set(GLOBAL_LOG_LEVEL SPDLOG_LEVEL_TRACE)
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=${GLOBAL_LOG_LEVEL})

# =============================================================
# Json
# =============================================================

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# =============================================================
# Core
# =============================================================

add_library(CoreTypes INTERFACE)

target_include_directories(CoreTypes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Enums
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Structs
)

# =============================================================
# DetectionPipeline
# =============================================================

add_library(DetectionPipeline
    DetectionPipeline/DetectionPipeline.cpp
    DetectionPipeline/01_Preprocessing/Preprocessing.cpp
    DetectionPipeline/02_Detection/Detection.cpp
	DetectionPipeline/02_Detection/AprilTagDetection/AprilTagDetection.cpp
	DetectionPipeline/02_Detection/ArUcoDetection/ArUcoDetection.cpp
    DetectionPipeline/03_PostProcessing/PostProcessing.cpp
)

target_include_directories(DetectionPipeline 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/DetectionPipeline
)

target_link_libraries(DetectionPipeline 
    PUBLIC
        AprilTag
        CoreTypes
        ${OpenCV_LIBS}
    PRIVATE
        ImageSource
)

# =============================================================
# ImageSource
# =============================================================

add_library(ImageSource
    IO/ImageSource/ImageSourceFactory.cpp
    IO/ImageSource/LiveImageSource/LiveImageSource.cpp
    IO/ImageSource/LiveImageSource/LUCIDStream/LUCIDStream.cpp
    IO/ImageSource/LiveImageSource/RTPStream/RTPStream.cpp
    IO/ImageSource/LiveImageSource/RTSPStream/RTSPStream.cpp
    IO/ImageSource/LiveImageSource/WebcamStream/WebcamStream.cpp
    IO/ImageSource/StaticImageSource/StaticImageSource.cpp
)

set(LUCID_ROOT $ENV{LUCID_DEV_ROOT})
set(LUCID_LIB_DIR "${LUCID_ROOT}/lib/Arena")
set(GENICAM_ROOT $ENV{LUCID_GENICAM_PATH})
set(GENICAM_LIB_DIR "${GENICAM_ROOT}/library/CPP/lib/Win64_x64")

target_include_directories(ImageSource 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/IO
    PRIVATE
        ${LUCID_ROOT}/include/Arena
        ${LUCID_ROOT}/include/GenTL
        ${LUCID_ROOT}/include/Save
        ${GENICAM_ROOT}/library/CPP/include
)

target_link_directories(ImageSource PUBLIC
	${LUCID_ROOT}/lib/win64
	${GENICAM_ROOT}/library/CPP/lib/win64
)

target_link_libraries(ImageSource 
    PUBLIC
        ${OpenCV_LIBS}
        Helper
        CoreTypes

    PRIVATE
        Arena
		Save
		GenICam
)

# =============================================================
# Sink
# =============================================================

add_library(Sink
    IO/Sink/Sink.cpp
    IO/Sink/ResultLogger.cpp
    IO/Sink/UdpPublisher.cpp
)

target_include_directories(Sink PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/IO/Sink
)

target_link_libraries(Sink PUBLIC
    sqlite3
    Helper
)

# =============================================================
# Supervisor
# =============================================================

add_library(Supervisor
    Supervisor/LiveSupervisorMode.cpp
    Supervisor/StaticSupervisorMode.cpp
    Supervisor/LiveWorker/Worker.cpp
)

target_include_directories(Supervisor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Supervisor
    ${CMAKE_CURRENT_SOURCE_DIR}/Supervisor/LiveWorker
)

target_link_libraries(Supervisor 
    PUBLIC
        ImageSource
        Sink
        SettingsReader
        Helper
        CoreTypes
        DetectionPipeline
)

# =============================================================
# Settings
# =============================================================

add_library(SettingsReader
    SettingsReader/SettingsReader.cpp
)

target_include_directories(SettingsReader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsReader
)

target_link_libraries(SettingsReader PUBLIC
    nlohmann_json
    Helper
    CoreTypes
)

# =============================================================
# Hauptprogramm
# =============================================================

add_executable(MarkerPositioning
    MarkerPositioning.cpp
)

target_link_libraries(MarkerPositioning PRIVATE
    Supervisor
)
