add_library(SettingsReader STATIC
	SettingsReader.cpp
	SettingsReader.h
	MainSettings.h)

target_include_directories(SettingsReader 
	PRIVATE 
		${CMAKE_CURRENT_SOURCE_DIR}/../external
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SettingsReader PRIVATE 
	Helper
	Detection
	ImageSource
)

target_compile_definitions(SettingsReader
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)