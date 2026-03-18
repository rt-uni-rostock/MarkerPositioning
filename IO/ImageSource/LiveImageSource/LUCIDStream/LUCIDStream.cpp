#include "LUCIDStream.h"
#include "ImageSource/ImageFrame.h"
#include "Logger.h"

LUCIDStream::LUCIDStream(const ImageSourceConfig& config) : IVideoStream(config) {
	system_ = Arena::OpenSystem();
}

LUCIDStream::~LUCIDStream() {
	Arena::CloseSystem(system_);
}

bool LUCIDStream::open() {
	system_->UpdateDevices(100);
	std::vector<Arena::DeviceInfo> deviceInfos = system_->GetDevices();
	if (deviceInfos.size() == 0)
	{
		LOG_ERROR("No camera connected");
		return false;
	}
	Arena::DeviceInfo selectedDeviceInfo = deviceInfos[0];
	Arena::IDevice* pDevice = system_->CreateDevice(selectedDeviceInfo);

	int64_t width = Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "Width");
	int64_t height = Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "Height");

	LOG_INFO("Selected LUCID camera with resolution (w,h) = ({},{})", width, height);
	// Implement the logic to open the connection to the LUCID camera

	system_->DestroyDevice(pDevice);

	return false;
}

bool LUCIDStream::close() {
	// Implement the logic to close the connection to the LUCID camera
	return false;
}

ImageFrame LUCIDStream::getFrame() {
	// Implement the logic to retrieve the latest frame from the LUCID camera
	// This is a placeholder implementation and should be replaced with actual camera interaction code
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();

	//Arena::IImage* pImage = pDevice->GetImage(TIMEOUT);


	latestData.frameId = frameCounter_++;
	latestData.image = cv::Mat(); // Replace with actual frame data from the camera
	return latestData;
}