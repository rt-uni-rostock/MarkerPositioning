include(${CMAKE_SOURCE_DIR}/config.cmake)

add_library(LiveImageSource STATIC
	LiveImageSource.cpp
	LiveImageSource.h
    IVideoStream.h
)

target_include_directories(LiveImageSource PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(RTSPStream)
add_subdirectory(RTPStream)
add_subdirectory(LUCIDStream)


target_link_libraries(LiveImageSource
    PUBLIC
        RTSPStream
        RTPStream
        LUCIDStream
)

target_link_libraries(LiveImageSource PRIVATE
	SettingsReader
    ImageSource
)

target_compile_definitions(LiveImageSource
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)