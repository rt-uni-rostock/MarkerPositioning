cmake_minimum_required (VERSION 3.16)

add_library (AprilTagDetection
	"AprilTagDetection.h"
	"AprilTagDetection.cpp"
 "AprilTagWithPose.h")

target_compile_features(AprilTagDetection PUBLIC cxx_std_20)

find_package(OpenCV REQUIRED)

target_include_directories(AprilTagDetection PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${OpenCV_INCLUDE_DIRS})

target_link_libraries(AprilTagDetection PUBLIC ${OpenCV_LIBS})

# include AprilTag library
if(WIN32)
	set(APRILTAG_INCLUDE_DIR "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag")
    set(APRILTAG_LIB "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag/build/Release/apriltag.lib")
	
	target_include_directories(AprilTagDetection PUBLIC ${APRILTAG_INCLUDE_DIR})
	#target_link_directories(AprilTagDetection PRIVATE
    #    "C:/Users/marti/Code/FiducialMarker/cpp_tests/AprilTagModule/apriltaglib/apriltag/build/Release/"
    #)

	target_link_libraries(AprilTagDetection PRIVATE ${APRILTAG_LIB})
else()
	find_path(APRILTAG_INCLUDE_DIR apriltag.h PATHS /usr/local/include /usr/include)
    find_library(APRILTAG_LIB apriltag PATHS /usr/local/lib /usr/lib)
	
	if(NOT APRILTAG_INCLUDE_DIR OR NOT APRILTAG_LIB)
        message(FATAL_ERROR "Could not find AprilTag library")
    endif()

	target_include_directories(AprilTagDetection PRIVATE ${APRILTAG_INCLUDE_DIR})
    target_link_libraries(AprilTagDetection PRIVATE ${APRILTAG_LIB})
endif()