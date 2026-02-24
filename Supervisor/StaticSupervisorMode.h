#pragma once

#include "ISupervisorMode.h"

class ImageSourceFactory;
class DetectionPipeline;
class Sink;
struct MainSettings;

class StaticSupervisorMode : public ISupervisorMode {
public:
    explicit StaticSupervisorMode(
		ImageSourceFactory& imgSource,
        DetectionPipeline& pipeline,
		Sink& sink,
		const MainSettings& settings
    );

    void start() override;
	void stop() override;
private:
	ImageSourceFactory& imgSource_;
    DetectionPipeline& pipeline_;
	Sink& sink_;
};