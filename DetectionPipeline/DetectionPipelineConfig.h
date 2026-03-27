#pragma once
#include <string>
#include <cstdint>

enum class DetectionType {
	AprilTag,
	ArUco
};

// Configuration object for the DetectionPipeline.

struct DetectionPipelineConfig {
	double tagSize = 0.0;
	int tagID = 0;
	int cameraId = 0;
	double fx = 0.0;
	double fy = 0.0;
	double cx = 0.0;
	double cy = 0.0;
	double d1 = 0.0;
	double d2 = 0.0;
	double d3 = 0.0;
	double d4 = 0.0;
	double d5 = 0.0;
	double quadDecimate = 0.0;
	DetectionType detectionType = DetectionType::AprilTag;
};