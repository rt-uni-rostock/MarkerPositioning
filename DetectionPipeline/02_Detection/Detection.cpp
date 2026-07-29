#include "Detection.h"
#include "DetectionResult.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionPipelineConfig.h"
#include "AprilTagDetection/AprilTagDetection.h"
#include "ArUcoDetection/ArUcoDetection.h"

#include "Logger.h"

// Constructor: 
Detection::Detection(const DetectionPipelineConfig& config)
{
	LOG_TRACE("Initialize Detection with config. Detection type: {}", static_cast<int>(config.detectionType));
	switch (config.detectionType) {
	case DetectionType::AprilTag:
		detector_ = std::make_unique<AprilTagDetection>(config);
		break;

	case DetectionType::ArUco:
		detector_ = std::make_unique<ArUcoDetection>(config);
		break;

	default:
		throw std::runtime_error("Unsupported detection type");
	}
}

Detection::~Detection()
{
	// Destructor implementation
}

DetectionResult Detection::process(const ImageFrame& frame)
{
	return detector_->process(frame);
}