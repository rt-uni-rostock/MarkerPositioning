add_library(Preprocessing STATIC
	Preprocessing.cpp
	Preprocessing.h
)

target_include_directories(Preprocessing PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Preprocessing
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)