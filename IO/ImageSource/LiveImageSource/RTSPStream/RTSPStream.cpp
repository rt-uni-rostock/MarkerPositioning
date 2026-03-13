#include "RTSPStream.h"

#include "ImageSource/ImageFrame.h"
#include "Logger.h"
#include "ImageSource/ImageSourceConfig.h"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

RTSPStream::RTSPStream(const ImageSourceConfig& settings) : settings_(settings)
{
	LOG_TRACE("RTSPStream created with provided settings: rtspUrl={}",
		settings.srcUrl);
}

RTSPStream::~RTSPStream()
{
	if (cap.isOpened())
	{
		LOG_WARN("RTSPStream destructor called but video capture is still open, releasing it now");
		cap.release();
		LOG_TRACE("Video capture released successfully in RTSPStream destructor");
	}

	LOG_TRACE("RTPStream is being destroyed.");
}

bool RTSPStream::open()
{
	LOG_TRACE("Opening RTSP stream...");

	cap.open(settings_.srcUrl); //, cv::CAP_FFMPEG

	if (!cap.isOpened())
	{
		LOG_ERROR("Could not open RTSP stream.");
		return false;
	}
	else
	{
		LOG_TRACE("RTSP stream opened successfully.");
		return true;
	}
}

bool RTSPStream::close()
{
	LOG_TRACE("RTSPStream is being destroyed, releasing video capture if opens");

	try {
		cap.release();
	}
	catch (...) {
		LOG_ERROR("Exception occurred while releasing video capture in RTSPStream close()");
		return false;
	}

	return true;
}

ImageFrame RTSPStream::getFrame()
{
	LOG_TRACE("Getting latest frame from RTSP stream...");

	if (!cap.isOpened())
	{
		LOG_ERROR("RTSP stream is not open, cannot get frame. Return empty frame.");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}
	cv::Mat frame;
	if (!cap.read(frame))
	{
		LOG_ERROR("Could not read frame from RTSP stream, return empty frame.");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}

	auto now = std::chrono::system_clock::now();
	auto formatted = fmt::format(fmt::runtime("{:%FT%TZ}"), now);
	LOG_TRACE("Frame read successfully from RTSP stream, timestamp={}, frameId={}", formatted, frameCounter_ + 1);
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frameId = frameCounter_++;
	latestData.image = frame;
	return latestData;
}