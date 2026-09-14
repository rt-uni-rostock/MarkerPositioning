#pragma once

#include "ISupervisorMode.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

class IImageSource;
class ImageUdpPublisher;
struct GeneralSettings;

class LiveImagePassthroughSupervisorMode : public ISupervisorMode {
public:
	explicit LiveImagePassthroughSupervisorMode(
		const std::vector<IImageSource*>& imgSources,
		const std::vector<uint8_t>& cameraIds,
		ImageUdpPublisher& imagePublisher,
		const GeneralSettings& settings
	);

	bool start() override;
	bool stop() override;

private:
	void supervisorLoop();
	void handleCycle();

	const std::vector<IImageSource*>& imgSources_;
	const std::vector<uint8_t> cameraIds_;
	ImageUdpPublisher& imagePublisher_;
	const GeneralSettings& settings_;

	std::chrono::milliseconds intervalMs_{ 0 };
	std::atomic<bool> running_{ false };
	std::thread supervisorThread_;
};
