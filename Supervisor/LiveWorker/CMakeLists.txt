add_library(Worker STATIC
	Worker.h
	Worker.cpp
)

target_include_directories(Worker PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Worker PRIVATE
	ImageSource
	Detection
	DetectionPipeline
	SettingsReader
	Helper
)

target_compile_definitions(Worker
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)