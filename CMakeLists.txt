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
# Feature Selection
# =============================================================
# ENABLE_LUCID steuert, ob Unterstuetzung fuer LUCID-GigE-Kameras (Arena SDK)
# einkompiliert wird. Manche Zielsysteme (z. B. Raspberry Pi) benoetigen keine
# LUCID-Kamera und sollen daher auch keine Arena-SDK-Installation benoetigen.
# Auswahl je Zielsystem z. B. ueber CMakePresets.json ("linux-debian" -> ON,
# "linux-rpi5" -> OFF) oder manuell per -DENABLE_LUCID=OFF.
option(ENABLE_LUCID "Enable LUCID GigE camera support (requires LUCID Arena SDK)" ON)

if(ENABLE_LUCID)
    message(STATUS "LUCID GigE camera support: ENABLED (ENABLE_LUCID=ON, Arena SDK required)")
else()
    message(STATUS "LUCID GigE camera support: DISABLED (ENABLE_LUCID=OFF, no Arena SDK required)")
endif()

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
else()
	# Unter Linux wird OpenCV i. d. R. selbst aus dem Quellcode gebaut (Debian-
	# Repository-Pakete sind meist zu alt bzw. ohne GStreamer/V4L2-Unterstuetzung).
	# OpenCV_DIR kann beim Konfigurieren ueberschrieben werden, z. B.:
	#   cmake -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 ..
	# Ist OpenCV_DIR nicht gesetzt, verwendet find_package() die Standard-Suchpfade
	# (u. a. /usr/local/lib/cmake/opencv4, sofern per "make install" installiert).
	if (DEFINED OpenCV_DIR)
		message(STATUS "Using OpenCV_DIR override: ${OpenCV_DIR}")
	endif()
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
    
    # Für DLL-Copy später
    set(APRILTAG_DLL
        "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag/build/Release/apriltag.dll"
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
    IO/ImageSource/LiveImageSource/RTPStream/RTPStream.cpp
    IO/ImageSource/LiveImageSource/RTSPStream/RTSPStream.cpp
    IO/ImageSource/LiveImageSource/WebcamStream/WebcamStream.cpp
    IO/ImageSource/StaticImageSource/StaticImageSource.cpp
)

target_include_directories(ImageSource 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/IO
)

target_link_libraries(ImageSource 
    PUBLIC
        ${OpenCV_LIBS}
        Helper
        CoreTypes
)

# -------------------------------------------------------------
# LUCID Arena SDK (GigE camera support), only if ENABLE_LUCID=ON
# -------------------------------------------------------------
if(ENABLE_LUCID)

    target_sources(ImageSource PRIVATE
        IO/ImageSource/LiveImageSource/LUCIDStream/LUCIDStream.cpp
    )

    # Compile definition used by ImageSourceFactory.cpp to conditionally
    # include/instantiate LUCIDStream.
    target_compile_definitions(ImageSource PUBLIC MP_ENABLE_LUCID)

    set(LUCID_ROOT $ENV{LUCID_DEV_ROOT})
    set(GENICAM_ROOT $ENV{LUCID_GENICAM_PATH})

    target_include_directories(ImageSource 
        PUBLIC
            ${LUCID_ROOT}/include/Arena
            ${LUCID_ROOT}/include/GenTL
            ${LUCID_ROOT}/include/Save
            ${GENICAM_ROOT}/library/CPP/include
    )

    if(WIN32)
        # Windows Arena SDK: pre-built C++ GenICam bindings under
        # GenICam/library/CPP/lib/Win64_x64, MSVC-versioned lib names.
        target_link_directories(ImageSource PUBLIC
            ${LUCID_ROOT}/lib64/Arena
            ${GENICAM_ROOT}/library/CPP/lib/Win64_x64
        )

        target_link_libraries(ImageSource PRIVATE
            Arenad_v140
            Saved_v140
        )
    else()
        # Linux Arena SDK (Debian x86_64 or ARM64, e.g. Raspberry Pi):
        # library layout/names differ from Windows, see LUCID's own
        # "Arena_SDK.conf"-based FindARENA.cmake reference implementation:
        #   <root>/lib64/libarena.so, libsave.so, libgentl.so
        #   <root>/GenICam/library/lib/Linux64_x64|Linux64_ARM/libGCBase_*.so, libGenApi_*.so
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
            set(ARENA_GENICAM_LIB_DIR "${GENICAM_ROOT}/library/lib/Linux64_ARM")
            set(ARENA_LIB_DIR "${LUCID_ROOT}/lib")
        else()
            set(ARENA_GENICAM_LIB_DIR "${GENICAM_ROOT}/library/lib/Linux64_x64")
            set(ARENA_LIB_DIR "${LUCID_ROOT}/lib64")
        endif()

        find_library(ARENA_LIB arena PATHS ${ARENA_LIB_DIR})
        find_library(ARENA_SAVE_LIB save PATHS ${ARENA_LIB_DIR})
        find_library(ARENA_GENTL_LIB gentl PATHS ${ARENA_LIB_DIR})

        # GCBase/GenApi shared library file names carry a compiler/version
        # suffix (e.g. libGCBase_gcc54_v3_3_LUCID.so) that varies by SDK
        # release, hence they are located via glob instead of a fixed name.
        file(GLOB ARENA_GENICAM_GCBASE_LIB "${ARENA_GENICAM_LIB_DIR}/libGCBase_*.so")
        file(GLOB ARENA_GENICAM_GENAPI_LIB "${ARENA_GENICAM_LIB_DIR}/libGenApi_*.so")

        if(NOT ARENA_LIB OR NOT ARENA_SAVE_LIB OR NOT ARENA_GENTL_LIB
                OR NOT ARENA_GENICAM_GCBASE_LIB OR NOT ARENA_GENICAM_GENAPI_LIB)
            message(FATAL_ERROR
                "ENABLE_LUCID=ON but the LUCID Arena SDK could not be found. "
                "Make sure LUCID_DEV_ROOT and LUCID_GENICAM_PATH are set (see "
                "docs/linux-setup.md) and the Arena SDK is installed, or "
                "configure with -DENABLE_LUCID=OFF to build without LUCID "
                "GigE camera support.")
        endif()

        target_link_libraries(ImageSource PRIVATE
            ${ARENA_LIB}
            ${ARENA_SAVE_LIB}
            ${ARENA_GENTL_LIB}
            ${ARENA_GENICAM_GCBASE_LIB}
            ${ARENA_GENICAM_GENAPI_LIB}
        )
    endif()
endif()

# =============================================================
# Sink
# =============================================================

add_library(Sink
    IO/Sink/ImageUdpPublisher.cpp
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
    DetectionPipeline
    ImageSource
)

# =============================================================
# Supervisor
# =============================================================

add_library(Supervisor
    Supervisor/LiveSupervisorMode.cpp
    Supervisor/LiveImagePassthroughSupervisorMode.cpp
    Supervisor/StaticSupervisorMode.cpp
    Supervisor/SupervisorFactory.cpp
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

# =============================================================
# Auto-Copy DLLs to Build Output (Windows)
# =============================================================

if (WIN32)
    # OpenCV DLLs
    set(OPENCV_BIN_DIR "C:/Code/opencv_build2/build/install/x64/vc17/bin")
    
    # Debug DLLs für Debug-Build
    if (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_CONFIGURATION_TYPES MATCHES Debug)
        add_custom_command(TARGET MarkerPositioning POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENCV_BIN_DIR}/opencv_world4120d.dll"
                "$<CONFIG>/opencv_world4120d.dll"
            COMMAND ${CMAKE_COMMAND} -E echo "✓ Copied opencv_world4120d.dll"
            COMMENT "Copying OpenCV Debug DLLs..."
        )
    endif()
    
    # Release DLLs für Release-Build
    add_custom_command(TARGET MarkerPositioning POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENCV_BIN_DIR}/opencv_world4120.dll"
            "$<CONFIG>/opencv_world4120.dll"
        COMMAND ${CMAKE_COMMAND} -E echo "✓ Copied opencv_world4120.dll"
        COMMENT "Copying OpenCV Release DLLs..."
    )
    
    # Kopiere alle OpenCV DLLs
    file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/*.dll")
    foreach(DLL ${OPENCV_DLLS})
        get_filename_component(DLL_NAME ${DLL} NAME)
        add_custom_command(TARGET MarkerPositioning POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "$<TARGET_FILE_DIR:MarkerPositioning>/${DLL_NAME}"
            VERBATIM
        )
    endforeach()
    
    # AprilTag DLL
    if (DEFINED APRILTAG_DLL AND EXISTS "${APRILTAG_DLL}")
        add_custom_command(TARGET MarkerPositioning POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${APRILTAG_DLL}"
                "$<TARGET_FILE_DIR:MarkerPositioning>/apriltag.dll"
            COMMAND ${CMAKE_COMMAND} -E echo "✓ Copied apriltag.dll"
            COMMENT "Copying AprilTag Runtime DLL..."
        )
    endif()
endif()
