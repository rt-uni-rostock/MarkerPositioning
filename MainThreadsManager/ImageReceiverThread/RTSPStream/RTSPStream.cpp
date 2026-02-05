#include "RTSPStream.h"

RTSPStream::RTSPStream(SettingsReader::MainSettings& settings) : Settings(settings)
{
	std::cout << "Opening RTSP stream at " << Settings.rtspUrl << " ..." << std::endl;

	cap.open(Settings.rtspUrl); //, cv::CAP_FFMPEG
	if (!cap.isOpened())
	{
		std::cerr << "Error: Could not open RTSP stream at " << Settings.rtspUrl << std::endl;
	}
	else
	{
		streamIsOpen = true;
	}
}

RTSPStream::~RTSPStream()
{
	cap.release();
}

FrameData RTSPStream::getFrame()
{
	if (!cap.isOpened())
	{
		return FrameData{ cv::Mat(), std::chrono::system_clock::now() };
	}
	cv::Mat frame;
	if (!cap.read(frame))
	{
		std::cerr << "Error: Could not read frame from RTSP stream." << std::endl;
		return FrameData{ cv::Mat(), std::chrono::system_clock::now() };
	}
	FrameData latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frame = frame;
	return latestData;
}