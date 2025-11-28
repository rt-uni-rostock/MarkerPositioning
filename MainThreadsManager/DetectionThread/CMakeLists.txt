include(${CMAKE_SOURCE_DIR}/config.cmake)

add_library(DetectionThread
	DetectionThread.cpp
	DetectionThread.h
	IDetection.h
 "Pose.h")

find_package(OpenCV REQUIRED)

target_include_directories(DetectionThread PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(DetectionThread PUBLIC ${OpenCV_LIBS})

# ---- Tag type selection ----
if(TAG_TYPE STREQUAL "APRILTAG")
    message(STATUS "DetectionThread: Using AprilTag")
    add_subdirectory(AprilTagDetection)

    target_compile_definitions(DetectionThread PRIVATE USE_APRILTAG)

    target_link_libraries(DetectionThread
        PUBLIC
            AprilTagDetection
    )

elseif(TAG_TYPE STREQUAL "ARUCO")
    message(STATUS "DetectionThread: Using ArUco")
    add_subdirectory(ArUcoDetection)

    target_compile_definitions(DetectionThread PRIVATE USE_ARUCO)

    target_link_libraries(DetectionThread
        PUBLIC
            ArUcoDetection
    )
else()
    message(FATAL_ERROR "Invalid TAG_TYPE: ${TAG_TYPE}")
endif()