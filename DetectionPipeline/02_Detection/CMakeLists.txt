add_subdirectory(AprilTagDetection)
add_subdirectory(ArUcoDetection)

add_library(Detection STATIC
	Detection.cpp
	Detection.h
	Pose.h 
	IDetection.h 
	TagTypeEnum.h)

target_include_directories(Detection PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Detection PUBLIC
	AprilTagDetection
	ArUcoDetection
)

target_compile_definitions(Detection
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)