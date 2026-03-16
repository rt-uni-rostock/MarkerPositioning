#pragma once

#include "ImageSource/ImageFrame.h"
#include "ImageSource/ImageSourceConfig.h"

class IVideoStream
{
public:
	explicit IVideoStream(const ImageSourceConfig& config) : config_(config) {}

	virtual ~IVideoStream() = default;

	virtual bool open() = 0;
	virtual bool close() = 0;

	virtual ImageFrame getFrame() = 0;

	const double getMaxCaptureFPS() const { return config_.maxCaptureFPS; }

protected:
	ImageSourceConfig config_;
};