#pragma once

#include "ImageSource/ImageFrame.h"

class IVideoStream
{
public:
	virtual ~IVideoStream() = default;

	virtual void open() = 0;
	virtual void close() = 0;

	virtual ImageFrame getFrame() = 0;
};