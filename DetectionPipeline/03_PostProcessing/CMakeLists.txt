add_library(PostProcessing STATIC
	PostProcessing.cpp
	PostProcessing.h
)

target_include_directories(PostProcessing PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(PostProcessing
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)