#pragma once

#include "ImageFrame.h"

class IImageSource
{
public:
	virtual ~IImageSource() = default;

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual ImageFrame getLatestFrame() = 0;
};