#pragma once
//#include "ArenaApi.h"
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

	ImageFrame getFrame() override;
private:
	uint64_t frameCounter_ = 0;

	//Arena::ISystem* pSystem;
};