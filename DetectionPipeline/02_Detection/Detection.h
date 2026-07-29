#pragma once

#include <memory>

struct DetectionPipelineConfig;
struct ImageFrame;
struct DetectionResult;

class IDetection;

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class Detection
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit Detection(const DetectionPipelineConfig& config);

	// Destructor: cleans up the main threads
	~Detection();

	DetectionResult process(const ImageFrame& frame);

private:
	std::unique_ptr<IDetection> detector_;

};