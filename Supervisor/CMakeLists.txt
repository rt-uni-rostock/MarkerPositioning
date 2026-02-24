add_subdirectory(LiveWorker)

add_library(Supervisor STATIC
	ISupervisorMode.h
	StaticSupervisorMode.h
	StaticSupervisorMode.cpp
	LiveSupervisorMode.h
	LiveSupervisorMode.cpp
 )

target_include_directories(Supervisor PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Supervisor PRIVATE
	ImageSource
	DetectionPipeline
	Sink
	SettingsReader
	Helper
)

target_link_libraries(Supervisor PUBLIC
	Worker
)

target_compile_definitions(Supervisor
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)