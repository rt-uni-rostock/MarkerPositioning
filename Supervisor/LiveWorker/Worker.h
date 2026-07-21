#pragma once

#include <thread>
#include <string>
#include <functional>

class IImageSource;
class DetectionPipeline;
struct PipelineResult;

enum class WorkerState
{
	Idle,
	Running
};

class Worker {
public:
	Worker(IImageSource& source, DetectionPipeline& pipeline, uint8_t cameraId);
	~Worker();

	bool isIdle() const;

	void start(uint64_t cycleId,
		std::function<void(const PipelineResult&)> onSuccess,
		std::function<void(std::string)> onError);
	std::chrono::steady_clock::time_point startTime() const;

	uint8_t getCameraId() const { return cameraId_; }
private:
	std::atomic<WorkerState> state_{ WorkerState::Idle };
	
	std::thread thread_;
	std::chrono::steady_clock::time_point startTime_;

	IImageSource& source_;
	DetectionPipeline& pipeline_;

	const uint8_t cameraId_;
};