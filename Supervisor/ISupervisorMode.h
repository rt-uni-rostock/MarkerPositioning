#pragma once

class ImageAcquisition;
class DetectionPipeline;
class Sink;

class ISupervisorMode {
public:
    virtual ~ISupervisorMode() = default;
    virtual void start() = 0;
	virtual void stop() = 0;
};