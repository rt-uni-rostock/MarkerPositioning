#pragma once

#include "DetectionPipelineConfig.h"

struct DetectionResult;
struct DetectionPipelineConfig;

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class PostProcessing
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit PostProcessing(const DetectionPipelineConfig& config);

	// Destructor: cleans up the main threads
	~PostProcessing();

	DetectionResult process(const DetectionResult& detectionResult);
private:
	const DetectionPipelineConfig& config_;
};