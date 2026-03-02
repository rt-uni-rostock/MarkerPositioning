#include "WebcamStream.h"
#include "Logger.h"
#include "ImageSource/ImageFrame.h"
#include "ImageSource/ImageSourceConfig.h"

WebcamStream::WebcamStream(const ImageSourceConfig& settings) : settings_(settings)
{
	LOG_TRACE("WebcamStream created with provided settings: rtspUrl={}",
		settings.rtspUrl);
}

WebcamStream::~WebcamStream()
{
	// check if video capture is still open and release it
	if (cap.isOpened())
	{
		LOG_WARN("WebcamStream destructor called but video capture is still open, releasing it now");
		cap.release();
		LOG_TRACE("Video capture released successfully in WebcamStream destructor");
	}
	LOG_TRACE("WebcamStream is being destroyed.");
}

void WebcamStream::open()
{
	LOG_TRACE("Opening webcam stream...");
	cap.open(0); // open default camera
	if (!cap.isOpened())
	{
		LOG_ERROR("Could not open webcam stream");
	}
	else
	{
		LOG_TRACE("Webcam stream opened successfully");
	}
}

void WebcamStream::close()
{
	LOG_TRACE("WebcamStream is being destroyed, releasing video capture if open");
	cap.release();
}

ImageFrame WebcamStream::getFrame()
{
	LOG_TRACE("Getting latest frame from WebcamStream...");
	if (!cap.isOpened())
	{
		LOG_ERROR("Webcam stream is not open, cannot get frame. Return empty frame.");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}
	cv::Mat frame;
	cap >> frame; // capture a new frame from the webcam
	if (frame.empty())
	{
		LOG_ERROR("Captured empty frame from webcam stream");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}
	LOG_TRACE("Successfully captured a new frame from webcam stream");
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.image = frame;
	latestData.frameId = frameCounter_++;

	return latestData;
}

