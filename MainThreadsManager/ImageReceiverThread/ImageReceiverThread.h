#pragma once
//#include <opencv2/opencv.hpp>
#include <chrono>
#include <mutex>
#include <thread>
#include "../../SettingsReader/SettingsReader.h"
#include "RTPStream/RTPStream.h"
#include "FrameData.h"
#include "IVideoStream.h"

class ImageReceiverThread
{
public:
	ImageReceiverThread(SettingsReader::MainSettings settings);
	~ImageReceiverThread();
	void startReceiving();
	void stopReceiving();
	bool running;

	FrameData getLatestFrame();
private:
	SettingsReader::MainSettings Settings;

	std::unique_ptr<IVideoStream> stream;

	void captureLoop(std::stop_token stopToken);

	std::jthread captureThread;
	std::mutex frameMutex;
	FrameData latestFrame;
};