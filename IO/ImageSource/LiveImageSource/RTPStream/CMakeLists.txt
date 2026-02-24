cmake_minimum_required (VERSION 3.16)

add_library (RTPStream STATIC
	RTPStream.h
	RTPStream.cpp
)

target_compile_features(RTPStream PUBLIC cxx_std_20)

find_package(OpenCV REQUIRED)

target_include_directories(RTPStream PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${OpenCV_INCLUDE_DIRS})

target_link_libraries(RTPStream PUBLIC ${OpenCV_LIBS})

target_link_libraries(RTPStream PUBLIC
	LiveImageSource
	Helper
	ImageSource
)

target_compile_definitions(RTPStream
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)