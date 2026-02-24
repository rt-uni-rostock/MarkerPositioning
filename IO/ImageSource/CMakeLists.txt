add_subdirectory(LiveImageSource)
add_subdirectory(StaticImageSource)

add_library(ImageSource STATIC
	IImageSource.h
	ImageSourceFactory.h
	ImageSourceFactory.cpp
	ImageFrame.h
	ImageSourceConfig.h 
	SourceModeEnum.h
	StreamTypeEnum.h)

find_package(OpenCV REQUIRED)

target_include_directories(ImageSource PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(ImageSource PRIVATE
	SettingsReader
	Helper
)

target_link_libraries(ImageSource PUBLIC
	LiveImageSource
	StaticImageSource
	${OpenCV_LIBS}
)

target_compile_definitions(ImageSource
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)