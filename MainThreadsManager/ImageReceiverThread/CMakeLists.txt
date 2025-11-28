include(${CMAKE_SOURCE_DIR}/config.cmake)

add_library(ImageReceiverThread
	ImageReceiverThread.cpp
	ImageReceiverThread.h
	FrameData.h
    "IVideoStream.h"
)

target_include_directories(ImageReceiverThread PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- Stream selection ----
if(VIDEO_STREAM STREQUAL "RTSP")
    message(STATUS "ImageReceiverThread: Using RTSPStream")
    add_subdirectory(RTSPStream)

    target_compile_definitions(ImageReceiverThread PRIVATE USE_RTSP)

    target_link_libraries(ImageReceiverThread
        PUBLIC
            RTSPStream
    )

elseif(VIDEO_STREAM STREQUAL "LUCID")
    message(STATUS "ImageReceiverThread: Using LUCIDStream")
    add_subdirectory(LUCIDStream)

    target_compile_definitions(ImageReceiverThread PRIVATE USE_LUCID)

    target_link_libraries(ImageReceiverThread
        PUBLIC
            LUCIDStream
    )
else()
    message(FATAL_ERROR "Invalid VIDEO_STREAM: ${VIDEO_STREAM}")
endif()