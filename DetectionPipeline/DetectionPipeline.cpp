#include "DetectionPipeline.h"

#include "DetectionResult.h"
#include "ImageSource/ImageFrame.h"

#include "01_Preprocessing/Preprocessing.h"
#include "02_Detection/Detection.h"
#include "03_PostProcessing/PostProcessing.h"

#include "Logger.h"

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

DetectionResult DetectionPipeline::process(ImageFrame& frame)
{
	// Preprocesing step
	LOG_INFO("PreProcessing frame with ID: {}", frame.frameId);
	ImageFrame preprocessedFrame = preprocessor_->process(frame);

	// Detection step

	LOG_INFO("Detecting in frame with ID: {}", frame.frameId);
	DetectionResult rawResult = detector_->process(preprocessedFrame);

	LOG_INFO("Detection completed for frame ID: {}, success: {}, marker ID: {}", frame.frameId, rawResult.success, rawResult.markerId);
	LOG_INFO("Raw detection result: posx={}, posy={}, posz={}, roll={}, pitch={}, yaw={}", rawResult.pose.x, rawResult.pose.y, rawResult.pose.z, rawResult.pose.roll, rawResult.pose.pitch, rawResult.pose.yaw);

	// Postprocessing step
	LOG_INFO("PostProcessing detection result for frame with ID: {}", frame.frameId);
	DetectionResult finalResult = postprocessor_->process(rawResult);

	LOG_TRACE("Finished processing frame with ID: {}", frame.frameId);
	return finalResult;
}