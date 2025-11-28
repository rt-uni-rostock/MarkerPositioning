#pragma once
#include "DetectionThread/DetectionThread.h"
#include "ImageReceiverThread/ImageReceiverThread.h"
#include "UDPSenderThread/UDPSenderThread.h"
#include "../SettingsReader/SettingsReader.h"
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>

class MainThreadsManager
{
public:
	MainThreadsManager(SettingsReader::MainSettings settings);
	~MainThreadsManager();
private:
	void triggerProcessing();
	void startImageReceiver();

	DetectionThread Detection;
	ImageReceiverThread ImageReceiver;
	UDPSenderThread UDPSender;
	
	SettingsReader::MainSettings Settings;
};