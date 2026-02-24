add_library(Sink STATIC
	Sink.cpp
	Sink.h
	PipelineResult.h
	SinkConfig.h
	LogEvent.h
	UdpPublisher.h
	UdpPublisher.cpp
	ResultLogger.h
	ResultLogger.cpp)

target_link_libraries(Sink PRIVATE 
	Helper
)

target_include_directories(Sink PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Sink PUBLIC
	sqlite3
)

target_compile_definitions(Sink
    PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)