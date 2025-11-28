add_library(MainThreadsManager
	MainThreadsManager.cpp
	MainThreadsManager.h
)

find_package(OpenCV REQUIRED)

target_include_directories(MainThreadsManager PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

add_subdirectory(ImageReceiverThread)
add_subdirectory(DetectionThread)
add_subdirectory(UDPSenderThread)

target_link_libraries(MainThreadsManager PUBLIC
	ImageReceiverThread
	DetectionThread
	UDPSenderThread
	${OpenCV_LIBS}
)