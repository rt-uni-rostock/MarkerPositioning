#include "StaticImageSource.h"

#include "Logger.h"


// Constructor: 
StaticImageSource::StaticImageSource()
{
	LOG_TRACE("StaticImageSource created with provided folder path");
}

StaticImageSource::~StaticImageSource()
{
	LOG_TRACE("StaticImageSource is being destroyed, stopping stream if running");

	if (running_) {
		LOG_TRACE("StaticImageSource is still running during destruction, stopping it now...");
		stop();
	}
}

void StaticImageSource::start()
{
	LOG_TRACE("Starting StaticImageSource...");
	if (running_) 
	{
		LOG_WARN("StaticImageSource is already running, start() call ignored");
		return; // already running
	}
	LOG_TRACE("StaticImageSource is starting.");
	running_ = true;
}

void StaticImageSource::stop()
{
	LOG_TRACE("Stopping StaticImageSource...");

	if (!running_) 
	{
		LOG_WARN("StaticImageSource is not running, stop() call ignored");
		return; // already stopped
	}

	running_ = false;
}

ImageFrame StaticImageSource::getLatestFrame()
{
	LOG_TRACE("Getting latest frame from StaticImageSource...");

	if (!running_) {
		LOG_ERROR("StaticImageSource is not running, cannot get latest frame");
		throw std::runtime_error("StaticImageSource is not ready");
	}

	LOG_TRACE("Returning dummy image");

	ImageFrame frame;
	frame.image = cv::Mat::zeros(480, 640, CV_8UC3); // black image of size 640x480
	frame.timestamp = std::chrono::system_clock::now();
	frame.frameId = index_;
	index_++;
	return frame;
}