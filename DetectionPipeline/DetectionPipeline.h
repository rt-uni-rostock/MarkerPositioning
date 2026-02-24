#pragma once

class ImageFrame;
struct DetectionResult;
struct DetectionPipelineConfig;

class DetectionPipeline {
public:
	explicit DetectionPipeline(const DetectionPipelineConfig& config);
	~DetectionPipeline();

	// Process an image frame and return the detection result
	DetectionResult process(const ImageFrame& frame);
};