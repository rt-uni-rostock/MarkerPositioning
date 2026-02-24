#pragma once

#include "IImageSource.h"
#include "ImageSourceConfig.h"

class ImageSourceFactory
{
public:
	explicit ImageSourceFactory(const ImageSourceConfig& config);

	std::unique_ptr<IImageSource> create();

private:
	ImageSourceConfig config_;
};