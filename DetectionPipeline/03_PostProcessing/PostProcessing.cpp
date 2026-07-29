#include "PostProcessing.h"
#include "DetectionResult.h"

// Constructor: 
PostProcessing::PostProcessing(const DetectionPipelineConfig& config) : config_(config)
{

}

PostProcessing::~PostProcessing()
{
	// Destructor implementation
}

DetectionResult PostProcessing::process(const DetectionResult& detectionResult)
{
	// Post-processing implementation
	return detectionResult;
}