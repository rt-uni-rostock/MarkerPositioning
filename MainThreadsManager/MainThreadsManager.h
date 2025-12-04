#pragma once
#include "DetectionThread/DetectionThread.h"
#include "ImageReceiverThread/ImageReceiverThread.h"
#include "UDPSenderThread/UDPSenderThread.h"
#include "../SettingsReader/SettingsReader.h"
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>

// The MainThreadsManager class manages the main threads for image receiving, detection, and UDP sending.
class MainThreadsManager
{
public:
	// Constructor: initializes the main threads with the given settings
	MainThreadsManager(SettingsReader::MainSettings settings);

	// Destructor: cleans up the main threads
	~MainThreadsManager();

private:
	// Main loop that triggers processing of images and sending of UDP packets
	void triggerProcessing();

	// Starts the image receiving thread
	void startImageReceiver();

	// defines the main threads
	DetectionThread Detection;
	ImageReceiverThread ImageReceiver;
	UDPSenderThread UDPSender;
	
	// settings instance
	SettingsReader::MainSettings Settings;
};