#pragma once    

#include "ISupervisorMode.h"
#include "Worker.h"
#include <chrono>
#include <thread>
#include <vector>

class IImageSource;
class DetectionPipeline;
class Sink;
class Worker;
struct GeneralSettings;

class LiveSupervisorMode : public ISupervisorMode {
public:
    
    explicit LiveSupervisorMode(
        const std::vector<IImageSource*>& imgSources,
        const std::vector<DetectionPipeline*>& pipelines,
		Sink& sink,
        const GeneralSettings& settings
    );

    bool start() override;
    bool stop() override;
	//void increaseNextRunAndStop();
	//void logCurrentTime();
private:
    void supervisorLoop();
    void handleCycle();
	Worker* acquireFreeWorker(uint8_t cameraId);
	bool isWorkerWithinDeadline(Worker* worker, uint64_t cycleId);
    

    const std::vector<IImageSource*>& imgSources_;
    const std::vector<DetectionPipeline*>& pipelines_;
	Sink& sink_;
	const GeneralSettings& settings_;

    std::chrono::milliseconds intervalMS_{ 0 };

    // only one list of workers, can be used for each source
	std::vector<std::unique_ptr<Worker>> workers_;

    std::atomic<bool> running_{ false };
	std::thread supervisorThread_;

	uint64_t cycleCount_{ 0 };
};

// was soll hier passieren?
// Video Stream empfangen
//      a) LiveImageSource (dadrin wird Thread gestartet) starten, der den Stream empfängt und Bilder bereitstellt
//      b1) in festen zyklen Bilder abrufen und weiterverarbeiten
// Pipeline mit empfangenen Bildern füttern
//      b2) nächster Zyklusschritt: Pipeline mit abgerufenen Bild füttern und Ergebnis erhalten
// Ergebnisse der Pipeline senden (UDP)
//	    b3) nächster Zyklusschritt: Ergebnisse der Pipeline über UDP senden
