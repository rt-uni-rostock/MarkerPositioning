add_library(Helper STATIC
	Logger.h
	Logger.cpp
)

target_include_directories(Helper PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Helper PUBLIC
	spdlog::spdlog
)

