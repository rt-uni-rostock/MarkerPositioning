#pragma once

struct ImageFrame;
struct DetectionResult;

class IDetection
{
public:
	virtual ~IDetection() = default;
	virtual DetectionResult process(const ImageFrame& frame) = 0;
};