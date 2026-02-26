#pragma once

#include <memory>
#include "DetectionPipelineConfig.h"

struct ImageFrame;
struct DetectionResult;

class Preprocessing;
class Detection;
class PostProcessing;

class DetectionPipeline {
public:
	explicit DetectionPipeline(const DetectionPipelineConfig& config);
	~DetectionPipeline();

	// Delete copy constructor and assignment operator to prevent copying of the pipeline
	DetectionPipeline(const DetectionPipeline&) = delete;
	DetectionPipeline& operator=(const DetectionPipeline&) = delete;

	// Process an image frame and return the detection result
	DetectionResult process(ImageFrame& frame);
private:
	DetectionPipelineConfig config_;
	
	std::unique_ptr<Preprocessing> preprocessor_;
	std::unique_ptr<Detection> detector_;
	std::unique_ptr<PostProcessing> postprocessor_;
};