#pragma once

#include <memory>
#include "DetectionPipelineConfig.h"

struct ImageFrame;
struct DetectionResult;
struct PipelineResult;

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

	// Process an image frame and return the pipeline result (contains list of detected markers)
	PipelineResult process(ImageFrame& frame);

	// Get the camera ID associated with this pipeline
	int getCameraId() const { return config_.cameraId; }
private:
	// Converts the internal DetectionResult into the external PipelineResult
	PipelineResult toPipelineResult(const DetectionResult& detectionResult) const;

	DetectionPipelineConfig config_;
	
	std::unique_ptr<Preprocessing> preprocessor_;
	std::unique_ptr<Detection> detector_;
	std::unique_ptr<PostProcessing> postprocessor_;
};