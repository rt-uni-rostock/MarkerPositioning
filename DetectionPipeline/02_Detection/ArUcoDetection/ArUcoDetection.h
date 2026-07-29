#pragma once

#include "02_Detection/IDetection.h"
#include "DetectionPipelineConfig.h"

class ArUcoDetection : public IDetection {
public:
	explicit ArUcoDetection(const DetectionPipelineConfig& config);
	
	~ArUcoDetection();

	DetectionResult process(const ImageFrame& frame) override;
private:
	DetectionPipelineConfig config_;
};