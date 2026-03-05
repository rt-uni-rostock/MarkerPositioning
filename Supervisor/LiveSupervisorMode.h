#pragma once    

#include "ISupervisorMode.h"
#include "Worker.h"
#include <chrono>
#include <thread>

class IImageSource;
class DetectionPipeline;
class Sink;
class Worker;
struct MainSettings;

class LiveSupervisorMode : public ISupervisorMode {
public:
    
    explicit LiveSupervisorMode(
        IImageSource& imgSource1,
		IImageSource& imgSource2,
        DetectionPipeline& pipeline,
		Sink& sink,
        const MainSettings& settings
    );

    void start() override;
    void stop() override;
	//void increaseNextRunAndStop();
	//void logCurrentTime();
private:
    void supervisorLoop();
    void handleCycle();
	Worker* acquireFreeWorker();
    

    IImageSource& imgSource1_;
	IImageSource& imgSource2_;
    DetectionPipeline& pipeline_;
	Sink& sink_;
	const MainSettings& settings_;

    std::chrono::milliseconds intervalMS_{ 0 };

	std::vector<std::unique_ptr<Worker>> workersSrc1_;
	std::vector<std::unique_ptr<Worker>> workersSrc2_;

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

// Letzter Stand: ich wollte die Intervalle berechnen und damit den Prozess steuern