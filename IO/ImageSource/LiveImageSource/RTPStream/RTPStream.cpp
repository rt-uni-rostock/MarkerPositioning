#include "RTPStream.h"
#include "Logger.h"
#include "ImageFrame.h"
#include "ImageSourceConfig.h"

RTPStream::RTPStream(const ImageSourceConfig& settings) : settings_(settings)
{
	LOG_TRACE("RTPStream created with provided settings: rtspUrl={}",
		settings.rtspUrl);
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

void RTPStream::open()
{
	LOG_TRACE("Opening Gstreamer RTP stream...");

	std::string pipeline =
		"udpsrc port=5600 caps=\"application/x-rtp, media=video, encoding-name=JPEG, payload=26\" ! "
		"rtpjpegdepay ! jpegdec ! videoconvert ! appsink";

	cap.open(pipeline, cv::CAP_GSTREAMER);
	if (!cap.isOpened())
	{
		LOG_ERROR("Could not open RTP stream");
	}
	else
	{
		LOG_TRACE("RTP stream opened successfully");
	}
}

void RTPStream::close()
{
	LOG_TRACE("RTPStream is being destroyed, releasing video capture if open");
	cap.release();
}

ImageFrame RTPStream::getFrame()
{
	LOG_TRACE("Getting latest frame from RTPStream...");

	if (!cap.isOpened())
	{
		LOG_ERROR("RTP stream is not open, cannot get frame. Return empty frame.");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}

	cv::Mat frame;
	if (!cap.read(frame))
	{
		LOG_ERROR("Could not read frame from RTP stream, returning empty frame");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}

	auto now = std::chrono::system_clock::now();
	auto formatted = std::format("{:%FT%TZ}", now);
	LOG_TRACE("Frame read successfully from RTP stream, timestamp={}, frameId={}", formatted, frameCounter_ + 1);
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frameId = frameCounter_++;
	latestData.image = frame;
	return latestData;
}