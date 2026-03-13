#pragma once

#include "ImageSource/ImageFrame.h"

class IVideoStream
{
public:
	virtual ~IVideoStream() = default;

	virtual bool open() = 0;
	virtual bool close() = 0;

	virtual ImageFrame getFrame() = 0;
};