#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>
#include "../../../SettingsReader/SettingsReader.h"
#include "../FrameData.h"
#include "../IVideoStream.h"

class RTSPStream : public IVideoStream
{
public:
	RTSPStream(SettingsReader::MainSettings& settings);
	~RTSPStream();

	FrameData getFrame() override;
private:
	SettingsReader::MainSettings Settings;

	cv::VideoCapture cap;
};