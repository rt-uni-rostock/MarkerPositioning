#include "RTPStream.h"

RTPStream::RTPStream(SettingsReader::MainSettings& settings) : Settings(settings)
{
	std::cout << "Opening RTP stream" << std::endl;

	std::string pipeline =
		"udpsrc port=5600 caps=\"application/x-rtp, media=video, encoding-name=JPEG, payload=26\" ! "
		"rtpjpegdepay ! jpegdec ! videoconvert ! appsink";

	cap.open(pipeline, cv::CAP_GSTREAMER);
	if (!cap.isOpened())
	{
		std::cerr << "Error: Could not open RTP stream";
	}
	else
	{
		streamIsOpen = true;
	}
}

RTPStream::~RTPStream()
{
	cap.release();
}

FrameData RTPStream::getFrame()
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