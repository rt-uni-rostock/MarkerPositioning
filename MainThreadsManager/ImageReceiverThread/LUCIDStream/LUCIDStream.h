#pragma once
//#include "ArenaApi.h"
#include "../../../SettingsReader/SettingsReader.h"
#include "../ImageReceiverThread.h"
#include "../IVideoStream.h"

class LUCIDStream : public IVideoStream {
public:
	LUCIDStream(SettingsReader::MainSettings settings);
	~LUCIDStream();

	FrameData getFrame() override;
private:
	SettingsReader::MainSettings Settings;

	//Arena::ISystem* pSystem;
};