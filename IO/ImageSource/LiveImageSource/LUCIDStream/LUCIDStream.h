#pragma once
#include "ArenaApi.h"
//#include "../ImageReceiverThread.h"
#include "ImageSource/LiveImageSource/IVideoStream.h"

class ImageFrame;
struct ImageSourceConfig;

class LUCIDStream : public IVideoStream {
public:
	explicit LUCIDStream(const ImageSourceConfig& config);
	~LUCIDStream();

	bool open() override;
	bool close() override;
	bool startStreaming() override;

	ImageFrame getFrame() override;
private:
	uint64_t frameCounter_ = 0;

	// Not owned by this instance: obtained from/released via
	// LUCIDSystemManager, which is the single process-wide holder of the
	// one Arena system the SDK allows (see LUCIDSystemManager.h).
	Arena::ISystem* system_ = nullptr;
	Arena::IDevice* device_ = nullptr;
	bool systemAcquired_ = false;
};