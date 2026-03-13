#pragma once
#include <opencv2/opencv.hpp>
#include <chrono>
#include "ImageSource/LiveImageSource/IVideoStream.h"

class ImageFrame;
struct ImageSourceConfig;

class RTSPStream : public IVideoStream
{
public:
	RTSPStream(const ImageSourceConfig& settings);
	~RTSPStream();

	bool open() override;
	bool close() override;

	ImageFrame getFrame() override;
private:
	const ImageSourceConfig& settings_;

	cv::VideoCapture cap;

	uint64_t frameCounter_ = 0;
};