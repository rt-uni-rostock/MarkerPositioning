add_library(StaticImageSource STATIC
	StaticImageSource.cpp
	StaticImageSource.h
	)

target_include_directories(StaticImageSource PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StaticImageSource PUBLIC
	SettingsReader
    ImageSource
	Helper
)

target_compile_definitions(StaticImageSource
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)