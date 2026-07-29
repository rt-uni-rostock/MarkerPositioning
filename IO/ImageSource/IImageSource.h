#pragma once

#include "ImageFrame.h"

class IImageSource
{
public:
	virtual ~IImageSource() = default;

	virtual bool start() = 0;
	virtual bool stop() = 0;
	virtual ImageFrame getLatestFrame() = 0;
};