#include "LiveImageSource.h"

#include "Logger.h"


// Constructor: 
LiveImageSource::LiveImageSource(std::unique_ptr<IVideoStream> stream) : stream_(std::move(stream))
{
	LOG_TRACE("LiveImageSource created with provided video stream");
}

LiveImageSource::~LiveImageSource()
{
	LOG_TRACE("LiveImageSource is being destroyed, stopping stream if running");
	
	if (running_) {
		LOG_TRACE("LiveImageSource is still running during destruction, stopping it now...");
		stop();
	}
}

void LiveImageSource::start()
{
	LOG_TRACE("Starting LiveImageSource...");
	if (running_) 
	{
		LOG_WARN("LiveImageSource is already running, start() call ignored");
		return; // already running
	}

	LOG_TRACE("Opening video stream...");
	stream_->open();
	running_ = true;

	// Start capture loop in a separate thread
	LOG_TRACE("Starting capture thread...");
	captureThread_ = std::thread(&LiveImageSource::captureLoop, this);
}

void LiveImageSource::stop()
{
	LOG_TRACE("Stopping LiveImageSource...");

	if (!running_) 
	{
		LOG_WARN("LiveImageSource is not running, stop() call ignored");
		return; // already stopped
	}

	running_ = false;

	LOG_TRACE("Stopping capture thread...");

	if (captureThread_.joinable()) {
		captureThread_.join();
		LOG_TRACE("Capture thread stopped successfully");
	}
	
	stream_->close();
	LOG_TRACE("Video stream closed successfully");
}

ImageFrame LiveImageSource::getLatestFrame()
{
	LOG_TRACE("Getting latest frame from LiveImageSource...");
	std::scoped_lock lock(frameMutex_);

	if (!running_) {
		LOG_ERROR("LiveImageSource is not running, cannot get latest frame");
		throw std::runtime_error("LiveImageSource is not running");
	}

	LOG_TRACE("Latest frame retrieved successfully");
	return latestFrame_;
}

void LiveImageSource::captureLoop()
{
	LOG_TRACE("Starting capture loop in LiveImageSource...");
	while (running_) {
		ImageFrame frame = stream_->getFrame();
		{
			std::scoped_lock lock(frameMutex_);
			latestFrame_ = frame;
		}
		LOG_TRACE("New frame captured and stored in LiveImageSource");
	}
}