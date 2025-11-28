#include "ImageReceiverThread.h"

#if defined(USE_RTSP)
#include "RTSPStream/RTSPStream.h"
#elif defined(USE_LUCID)
#include "LUCIDStream/LUCIDStream.h"
#endif

ImageReceiverThread::ImageReceiverThread(SettingsReader::MainSettings settings) : running(false)
{
	Settings = settings;

#if defined(USE_RTSP)
	stream = std::make_unique<RTSPStream>(settings);
#elif defined(USE_LUCID)
	stream = std::make_unique<LUCIDStream>(settings);
#endif
}

ImageReceiverThread::~ImageReceiverThread()
{
	stopReceiving();
}

void ImageReceiverThread::startReceiving()
{
	if (stream->streamIsOpen) {
		running = true;

		captureThread = std::jthread([this](std::stop_token st) {captureLoop(st); });
	}
}

void ImageReceiverThread::stopReceiving()
{
	running = false;

	captureThread.request_stop();
}

FrameData ImageReceiverThread::getLatestFrame()
{
	std::scoped_lock lock(frameMutex);

	if (!running) {
		throw std::runtime_error("Frame receiver is not running. Cannot get latest frame.");
	}

	return latestFrame;
}

void ImageReceiverThread::captureLoop(std::stop_token stopToken)
{
	while (!stopToken.stop_requested())
	{
		FrameData emptyData = { cv::Mat(), std::chrono::system_clock::time_point{} };

		FrameData latestData = stream->getFrame();

		{
			std::scoped_lock lock(frameMutex);
			latestFrame = latestData;
		}

		//std::cout << "Captured frame at " << std::chrono::duration_cast<std::chrono::milliseconds>(latestData.timestamp.time_since_epoch()).count() << " ms" << std::endl;
	}
}