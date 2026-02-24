#include "StaticSupervisorMode.h"
#include "ImageSourceFactory.h"
#include "DetectionPipeline.h"
#include "Sink.h"
#include "MainSettings.h"

//#include "ImageSourceFactory.h"
//#include "DetectionPipeline/DetectionPipeline.h"
//#include "IO/Sink/Sink.h"
//#include "SettingsReader/MainSettings.h"

StaticSupervisorMode::StaticSupervisorMode(
	ImageSourceFactory& imgSource,
	DetectionPipeline& pipeline,
	Sink& sink,
	const MainSettings& settings
) : imgSource_(imgSource), pipeline_(pipeline), sink_(sink)
{
}

void StaticSupervisorMode::start() {}

void StaticSupervisorMode::stop() {}