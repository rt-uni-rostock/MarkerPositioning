add_subdirectory(01_Preprocessing)
add_subdirectory(02_Detection)
add_subdirectory(03_PostProcessing)

add_library(DetectionPipeline STATIC
	DetectionPipeline.cpp
	DetectionPipeline.h
	DetectionPipelineConfig.h 
	DetectionResult.h)

target_include_directories(DetectionPipeline PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(DetectionPipeline PRIVATE
	ImageSource
	Helper
)

target_compile_definitions(DetectionPipeline
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)