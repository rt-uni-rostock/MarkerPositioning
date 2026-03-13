#pragma once
//#include "ArenaApi.h"
//#include "../ImageReceiverThread.h"
#include "ImageSource/LiveImageSource/IVideoStream.h"

class ImageFrame;
struct ImageSourceConfig;

class LUCIDStream : public IVideoStream {
public:
	LUCIDStream(const ImageSourceConfig& settings);
	~LUCIDStream();

	bool open() override;
	bool close() override;

	ImageFrame getFrame() override;
private:
	const ImageSourceConfig& settings_;

	uint64_t frameCounter_ = 0;

	//Arena::ISystem* pSystem;
};