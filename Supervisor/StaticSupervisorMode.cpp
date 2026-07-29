#include "StaticSupervisorMode.h"
#include "ImageSource/ImageSourceFactory.h"
#include "DetectionPipeline.h"
#include "Sink/Sink.h"
#include "GeneralSettings.h"

//#include "StaticSupervisorMode.h"
//#include "ImageSourceFactory.h"
//#include "DetectionPipeline.h"
//#include "Sink.h"
//#include "MainSettings.h"

//#include "ImageSourceFactory.h"
//#include "DetectionPipeline/DetectionPipeline.h"
//#include "IO/Sink/Sink.h"
//#include "SettingsReader/MainSettings.h"

StaticSupervisorMode::StaticSupervisorMode(
	ImageSourceFactory& imgSource,
	DetectionPipeline& pipeline,
	Sink& sink,
	const GeneralSettings& settings
) : imgSource_(imgSource), pipeline_(pipeline), sink_(sink)
{
}

bool StaticSupervisorMode::start() {
	return false;
}

bool StaticSupervisorMode::stop() {
	return false;
}