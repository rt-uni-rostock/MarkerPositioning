#include "DetectionPipeline.h"

#include "DetectionResult.h"
#include "ImageFrame.h"


// Constructor: 
DetectionPipeline::DetectionPipeline(const DetectionPipelineConfig& config)
{

}

DetectionPipeline::~DetectionPipeline()
{
	// Destructor implementation
}

DetectionResult DetectionPipeline::process(const ImageFrame& frame)
{
	DetectionResult result;
	result.success = false;
	result.markerId = -1;
	result.message = "Detection not implemented yet";
	return result;
}