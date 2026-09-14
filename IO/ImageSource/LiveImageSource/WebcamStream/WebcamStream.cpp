#include "WebcamStream.h"
#include "Logger.h"
#include "ImageSource/ImageFrame.h"
#include "ImageSource/ImageSourceConfig.h"
#include <array>

WebcamStream::WebcamStream(const ImageSourceConfig& config) : IVideoStream(config)
{
	LOG_TRACE("WebcamStream created with provided settings: rtspUrl={}",
		config_.cameraSettings->url);
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

bool WebcamStream::open()
{
	LOG_TRACE("Opening webcam stream...");

	const int configuredIndex = static_cast<int>(config_.cameraSettings->id);
	const std::array<int, 2> indices{ configuredIndex, 0 };
	const std::array<int, 3> backends{ cv::CAP_DSHOW, cv::CAP_MSMF, cv::CAP_ANY };

	bool opened = false;
	int openedIndex = -1;
	int openedBackend = cv::CAP_ANY;

	for (int backend : backends) {
		for (int index : indices) {
			if (opened) {
				break;
			}

			if (index < 0) {
				continue;
			}

			LOG_INFO("Trying webcam open with index {} and backend {}...", index, backend);
			if (cap.open(index, backend)) {
				opened = true;
				openedIndex = index;
				openedBackend = backend;
			}
		}
	}

	if (!cap.isOpened())
	{
		LOG_ERROR("Could not open webcam stream");
		return false;
	}
	else
	{
		// Keep UDP chunk count manageable for real-time passthrough.
		cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
		cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
		LOG_INFO("Webcam stream opened successfully with index {} and backend {}.", openedIndex, openedBackend);
		LOG_TRACE("Webcam stream opened successfully");
		return true;
	}
}

bool WebcamStream::close()
{
	LOG_TRACE("WebcamStream is being destroyed, releasing video capture if open");

	try {
		cap.release();
	}
	catch (...) {
		LOG_ERROR("Exception occurred while releasing video capture in WebcamStream close()");
		return false;
	}

	return true;
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
	//cap >> frame; // capture a new frame from the webcam
	if (!cap.read(frame))
	{
		LOG_WARN("Captured empty frame from webcam stream");
		return ImageFrame{ cv::Mat(), std::chrono::system_clock::now() };
	}
	LOG_TRACE("Successfully captured a new frame from webcam stream");
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.image = frame;
	latestData.frameId = frameCounter_++;

	return latestData;
}
