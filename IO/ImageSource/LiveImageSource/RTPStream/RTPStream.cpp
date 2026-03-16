#include "RTPStream.h"
#include "Logger.h"
#include "ImageSource/ImageFrame.h"
#include "ImageSource/ImageSourceConfig.h"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

RTPStream::RTPStream(const ImageSourceConfig& config) : IVideoStream(config)
{
	LOG_TRACE("RTPStream created with provided settings: url={}",
		config_.cameraSettings.url);
}

RTPStream::~RTPStream()
{
	// check if video capture is still open and release it
	if (cap.isOpened())
	{
		LOG_WARN("RTPStream destructor called but video capture is still open, releasing it now");
		cap.release();
		LOG_TRACE("Video capture released successfully in RTPStream destructor");
	}

	LOG_TRACE("RTPStream is being destroyed.");
}

bool RTPStream::open()
{
	LOG_TRACE("Opening Gstreamer RTP stream...");

	std::string pipeline =
		std::string("v4l2src device=/dev/video") + std::to_string(config_.cameraSettings.id) + std::string(" do-timestamp=true ! image/jpeg,width=1600,height=1200,framerate=15/1 ! jpegdec ! tee name=t t. ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=appsink drop=true max-buffers=2 sync=false t. ! queue ! videoconvert ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=15 bframes=0 ! rtph264pay config-interval=1 pt=96 ! ") +
		std::string("udpsink host=192.168.3.35 port=560") + std::to_string(config_.cameraSettings.id) + " sync = false async = false";

	cap.open(pipeline, cv::CAP_GSTREAMER);
	if (!cap.isOpened())
	{
		LOG_ERROR("Could not open RTP stream");
		return false;
	}
	else
	{
		LOG_TRACE("RTP stream opened successfully");
		return true;
	}
}

bool RTPStream::close()
{
	LOG_TRACE("RTPStream is being destroyed, releasing video capture if open");
	try {
		cap.release();
	}
	catch (...) {
		LOG_ERROR("Exception occurred while releasing video capture in RTPStream close().");
		return false;
	}

	return true;
}

ImageFrame RTPStream::getFrame()
{
	LOG_TRACE("Getting latest frame from RTPStream...");

	if (!cap.isOpened())
	{
		LOG_ERROR("RTP stream is not open, cannot get frame. Return empty frame.");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}

	cv::Mat frame;
	if (!cap.read(frame))
	{
		LOG_ERROR("Could not read frame from RTP stream, returning empty frame");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}

	auto now = std::chrono::system_clock::now();
	auto formatted = fmt::format(fmt::runtime("{:%FT%TZ}"), now);
	LOG_INFO("Frame read successfully from RTP stream, timestamp={}, frameId={}", formatted, frameCounter_ + 1);
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frameId = frameCounter_++;
	latestData.image = frame;
	return latestData;
}