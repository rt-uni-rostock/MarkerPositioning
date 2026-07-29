#pragma once

struct DetectionPipelineConfig;
struct ImageFrame;

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class Preprocessing
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit Preprocessing(const DetectionPipelineConfig& config);

	// Destructor: cleans up the main threads
	~Preprocessing();

	// Process an image frame and return the processed frame
	ImageFrame process(const ImageFrame& frame);

private:
	const DetectionPipelineConfig& config_;
};