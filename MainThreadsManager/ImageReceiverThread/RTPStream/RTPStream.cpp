#include "RTPStream.h"

RTPStream::RTPStream(SettingsReader::MainSettings& settings) : Settings(settings)
{
	std::cout << "Opening RTP stream" << std::endl;

	std::string pipeline =
		"v4l2src device=/dev/video2 do-timestamp=true ! "
		"image/jpeg,width=1600,height=1200,framerate=15/1 ! "
		"jpegdec ! "
		"tee name=t "

		"t. ! queue ! "
		"videoconvert ! video/x-raw,format=BGR ! "
		"appsink name=appsink drop=true max-buffers=2 sync=false "

		"t. ! queue ! "
		"videoconvert ! "
		"x264enc tune=zerolatency speed-preset=ultrafast bitrate=2000 key-int-max=15 bframes=0 ! "
		"rtph264pay config-interval=1 pt=96 ! "
		"udpsink host=192.168.3.35 port=5602 sync=false async=false";

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