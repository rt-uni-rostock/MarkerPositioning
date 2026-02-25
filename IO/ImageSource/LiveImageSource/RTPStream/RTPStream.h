#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>
#include "ImageSource/LiveImageSource/IVideoStream.h"
#include <iostream>

class ImageFrame;
struct ImageSourceConfig;

class RTPStream : public IVideoStream
{
public:
	RTPStream(const ImageSourceConfig& settings);
	~RTPStream();

	void open() override;
	void close() override;

	ImageFrame getFrame() override;

private:
	const ImageSourceConfig& settings_;

	cv::VideoCapture cap;

	uint64_t frameCounter_ = 0;
};