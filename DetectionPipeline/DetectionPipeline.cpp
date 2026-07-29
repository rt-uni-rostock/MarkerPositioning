#include "DetectionPipeline.h"

#include "DetectionResult.h"
#include "PipelineResult.h"
#include "ImageSource/ImageFrame.h"

#include "01_Preprocessing/Preprocessing.h"
#include "02_Detection/Detection.h"
#include "03_PostProcessing/PostProcessing.h"

#include "Logger.h"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

// Constructor: 
DetectionPipeline::DetectionPipeline(const DetectionPipelineConfig& config) : config_(config)
{
	LOG_TRACE("Initializing DetectionPipeline with config.");
	preprocessor_ = std::make_unique<Preprocessing>(config_);
	detector_ = std::make_unique<Detection>(config_);
	postprocessor_ = std::make_unique<PostProcessing>(config_);
}

DetectionPipeline::~DetectionPipeline()
{
	LOG_TRACE("Destroying DetecitonPipeline.");
	// Destructor implementation
}

PipelineResult DetectionPipeline::process(ImageFrame& frame)
{
	// Preprocesing step
	LOG_INFO("PreProcessing frame with ID: {}", frame.frameId);
	ImageFrame preprocessedFrame = preprocessor_->process(frame);

	// Detection step
	LOG_INFO("Detecting in frame with ID: {}", frame.frameId);
	DetectionResult rawResult = detector_->process(preprocessedFrame);

	LOG_INFO("Detection completed for frame ID: {}, success: {}, marker count: {}",
		frame.frameId, rawResult.success, rawResult.detectedMarkers.size());

	// Postprocessing step
	LOG_INFO("PostProcessing detection result for frame with ID: {}", frame.frameId);
	DetectionResult finalResult = postprocessor_->process(rawResult);

	LOG_TRACE("Finished processing frame with ID: {}", frame.frameId);

	return toPipelineResult(finalResult);
}

// Converts a DetectionResult (internal, detection-centric) into a PipelineResult
// (external, transport structure passed to supervisor/sink).
PipelineResult DetectionPipeline::toPipelineResult(const DetectionResult& detectionResult) const
{
	PipelineResult result;
	result.imageTimestamp = fmt::format(fmt::runtime("{:%FT%TZ}"), detectionResult.timestamp);
	result.cameraId = config_.cameraId;
	result.markerType = 0;
	result.errorCode = detectionResult.success ? 0 : 1;
	result.errorMessage = detectionResult.success ? "" : detectionResult.message;
	result.detectedMarkers = detectionResult.detectedMarkers;

	return result;
}