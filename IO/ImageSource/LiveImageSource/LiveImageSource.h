#pragma once

#include <thread>
#include <atomic>
#include <mutex>

#include "ImageSource/IImageSource.h"
#include "IVideoStream.h"

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class LiveImageSource : public IImageSource
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit LiveImageSource(std::unique_ptr<IVideoStream> stream);

	// Destructor: cleans up the main threads
	~LiveImageSource();

	bool start() override;
	bool stop() override;
	ImageFrame getLatestFrame() override;

private:
	void captureLoop();

	std::unique_ptr<IVideoStream> stream_;

	std::thread captureThread_;
	std::atomic<bool> running_ = false;

	std::mutex frameMutex_;
	ImageFrame latestFrame_;
};