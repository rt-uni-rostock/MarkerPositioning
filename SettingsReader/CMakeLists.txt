add_library(SettingsReader
	SettingsReader.cpp
	SettingsReader.h
)

target_include_directories(SettingsReader 
	PRIVATE 
		${CMAKE_CURRENT_SOURCE_DIR}/../external
)