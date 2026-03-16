#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>
#include "ImageSource/LiveImageSource/IVideoStream.h"
#include <iostream>

class ImageFrame;
struct ImageSourceConfig;

class WebcamStream : public IVideoStream
{
public:
	explicit WebcamStream(const ImageSourceConfig& config);
	~WebcamStream();

	bool open() override;
	bool close() override;

	ImageFrame getFrame() override;

private:

	cv::VideoCapture cap;

	uint64_t frameCounter_ = 0;
};