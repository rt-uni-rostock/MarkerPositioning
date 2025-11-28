#pragma once
#include "FrameData.h"

class IVideoStream
{
public:
	virtual ~IVideoStream() = default;
	virtual FrameData getFrame() = 0;
	bool streamIsOpen = false;
};