#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>
#include "../../../SettingsReader/SettingsReader.h"
#include "../FrameData.h"
#include "../IVideoStream.h"
#include <iostream>

class RTPStream : public IVideoStream
{
public:
	RTPStream(SettingsReader::MainSettings& settings);
	~RTPStream();

	FrameData getFrame() override;
private:
	SettingsReader::MainSettings Settings;

	cv::VideoCapture cap;
};