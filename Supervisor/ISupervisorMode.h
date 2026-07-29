#pragma once

class ImageAcquisition;
class DetectionPipeline;
class Sink;

class ISupervisorMode {
public:
    virtual ~ISupervisorMode() = default;
    virtual bool start() = 0;
	virtual bool stop() = 0;
};