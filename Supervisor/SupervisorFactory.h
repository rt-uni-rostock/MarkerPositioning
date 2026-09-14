#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "GeneralSettings.h"

class ISupervisorMode;
class IImageSource;
class DetectionPipeline;
class Sink;
class ImageUdpPublisher;

class SupervisorFactory {
public:
	explicit SupervisorFactory(const GeneralSettings& settings);
	~SupervisorFactory();

	std::unique_ptr<ISupervisorMode> create();

private:
	void initializeLiveDetectionDependencies();
	void initializeLiveImagePassthroughDependencies();
	std::vector<const CameraSettings*> collectActiveCameras() const;

	GeneralSettings settings_;

	std::vector<std::unique_ptr<IImageSource>> imgSources_;
	std::vector<IImageSource*> imgSourcePtrs_;
	std::vector<uint8_t> activeCameraIds_;

	std::vector<std::unique_ptr<DetectionPipeline>> pipelines_;
	std::vector<DetectionPipeline*> pipelinePtrs_;

	std::unique_ptr<Sink> sink_;
	std::unique_ptr<ImageUdpPublisher> imageUdpPublisher_;
};
