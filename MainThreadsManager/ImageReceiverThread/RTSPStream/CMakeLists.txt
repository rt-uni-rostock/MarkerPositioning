cmake_minimum_required (VERSION 3.16)

add_library (RTSPStream
	"RTSPStream.h"
	"RTSPStream.cpp"
)

target_compile_features(RTSPStream PUBLIC cxx_std_20)

find_package(OpenCV REQUIRED)

target_include_directories(RTSPStream PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${OpenCV_INCLUDE_DIRS})

target_link_libraries(RTSPStream PUBLIC ${OpenCV_LIBS})