cmake_minimum_required (VERSION 3.16)

add_library (LUCIDStream
	"LUCIDStream.h"
	"LUCIDStream.cpp"
)

target_compile_features(LUCIDStream PUBLIC cxx_std_20)

target_include_directories(LUCIDStream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# include LUCID SDK

if (NOT DEFINED ENV{LUCID_DEV_ROOT})
	message(FATAL_ERROR "LUCID_DEV_ROOT environment variable is not set. Please set it to the LUCID SDK root directory.")
endif()

if (NOT DEFINED ENV{LUCID_GENICAM_PATH})
	message(FATAL_ERROR "LUCID_GENICAM_PATH environment variable is not set. Please set it to the GenICam path provided by LUCID SDK.")
endif()

set(LUCID_ROOT $ENV{LUCID_DEV_ROOT})
set(GENICAM_ROOT $ENV{LUCID_GENICAM_PATH})

target_include_directories(LUCIDStream PUBLIC
	${LUCID_ROOT}/include/Arena
	${LUCID_ROOT}/include/GenTL
	${LUCID_ROOT}/include/Save
	${GENICAM_ROOT}/library/CPP/include
)

# library directories

if (WIN32)
	target_link_directories(LUCIDStream PUBLIC
		${LUCID_ROOT}/lib/win64
		${GENICAM_ROOT}/library/CPP/lib/win64
	)
elseif(UNIX)
	target_link_directories(LUCIDStream PUBLIC
		${LUCID_ROOT}/lib64
		${GENICAM_ROOT}/library/CPP/lib64
	)
endif()

# link libraries

if (WIN32)
	target_link_libraries(LUCIDStream PRIVATE
		Arena
		Save
		GenICam
	)
elseif(UNIX)
	target_link_libraries(LUCIDStream PRIVATE
		Arena
		Save
		GenICamBase
		GenApi
	)
endif()

target_compile_definitions(LUCIDStream PUBLIC USE_LUCID_STREAM)