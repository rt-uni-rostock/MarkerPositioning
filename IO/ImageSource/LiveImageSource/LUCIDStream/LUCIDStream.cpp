#include "LUCIDStream.h"
#include "ImageSource/ImageFrame.h"
#include "Logger.h"

LUCIDStream::LUCIDStream(const ImageSourceConfig& config) : IVideoStream(config) {
	LOG_TRACE("LUCIDStream created with provided settings: mode={}, streamType={}",
		static_cast<int>(config_.mode), static_cast<int>(config_.cameraSettings->streamType));
}

LUCIDStream::~LUCIDStream() {

	//TODO check if stream is still open and close it if necessary

	LOG_TRACE("LUCIDStream is being destroyed, closing system and cleaning up resources");
	Arena::CloseSystem(system_);
}

bool LUCIDStream::open() {
	
	LOG_TRACE("Opening LUCID camera stream...");
	system_ = Arena::OpenSystem();

	// TODO: check if system_ is not null and log error if it is null

	// get list of devices
	LOG_TRACE("Updating devices on system to discover connected cameras...");
	system_->UpdateDevices(100);
	std::vector<Arena::DeviceInfo> deviceInfos = system_->GetDevices();

	if (deviceInfos.size() == 0)
	{
		LOG_ERROR("No camera connected");
		return false;
	}

	// select first device and create device
	LOG_TRACE("Number of LUCID cameras detected: {}", deviceInfos.size());
	LOG_TRACE("Selecting first detected camera...");
	Arena::DeviceInfo selectedDeviceInfo = deviceInfos[0];
	LOG_TRACE("Creating device...");
	device_ = system_->CreateDevice(selectedDeviceInfo);

	// get and log camera resolution for test purposes
	int64_t width = Arena::GetNodeValue<int64_t>(device_->GetNodeMap(), "Width");
	int64_t height = Arena::GetNodeValue<int64_t>(device_->GetNodeMap(), "Height");
	LOG_INFO("Selected LUCID camera with resolution (w,h) = ({},{})", width, height);

	LOG_TRACE("Setting nodes for streaming...");
	GenICam::gcstring acquisitionModeInitial = Arena::GetNodeValue<GenICam::gcstring>(device_->GetNodeMap(), "AcquisitionMode");

	Arena::SetNodeValue<GenICam::gcstring>(
		device_->GetNodeMap(),
		"AcquisitionMode",
		"Continuous");

	Arena::SetNodeValue<GenICam::gcstring>(
		device_->GetTLStreamNodeMap(),
		"StreamBufferHandlingMode",
		"NewestOnly");

	Arena::SetNodeValue<bool>(
		device_->GetTLStreamNodeMap(),
		"StreamAutoNegotiatePacketSize",
		true);

	LOG_TRACE("Starting stream...");
	device_->StartStream();

	return true;
}

bool LUCIDStream::close() {
	LOG_TRACE("Stopping LUCID camera stream...");
	if (system_) {
		if (device_) {
			system_->DestroyDevice(device_);
		}
	}

	// Implement the logic to close the connection to the LUCID camera
	return true;
}

ImageFrame LUCIDStream::getFrame() {
	// Implement the logic to retrieve the latest frame from the LUCID camera
	// This is a placeholder implementation and should be replaced with actual camera interaction code
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();

	LOG_INFO("Retrieving latest frame from LUCID camera, frameId={}", frameCounter_ + 1);
	Arena::IImage* image_ = device_->GetImage(2000);

	// Konvertiere das Bild zu BGR8 Format falls nötig
	LOG_TRACE("Checking pixel format of retrieved image...");
	Arena::IImage* pConverted = nullptr;
	if (image_->GetPixelFormat() != BGR8)
	{
		pConverted = Arena::ImageFactory::Convert(image_, BGR8);
	}
	else
	{
		pConverted = image_;
	}

	// Berechne Stride manuell für BGR8 (3 Bytes pro Pixel)
	size_t stride = pConverted->GetWidth() * 3;

	LOG_TRACE("Converting image data to OpenCV Mat format...");
	cv::Mat frame(
		static_cast<int>(pConverted->GetHeight()),
		static_cast<int>(pConverted->GetWidth()),
		CV_8UC3,
		const_cast<uint8_t*>(pConverted->GetData()),
		stride
	);

	// save image frame for debugging
	//auto t = std::chrono::system_clock::to_time_t(frame.timestamp);
	/*std::stringstream filename;
	filename << "testimage.png";

	cv::imwrite(filename.str(), frame);*/

	LOG_TRACE("Successfully retrieved and converted image from LUCID camera, now copy and cleanup...");
	latestData.frameId = frameCounter_++;
	frame.copyTo(latestData.image);

	// Cleanup
	if (pConverted != image_)
	{
		Arena::ImageFactory::Destroy(pConverted);
	}
	device_->RequeueBuffer(image_);

	LOG_TRACE("Returning successfully retrieved frame from LUCID camera...");

	return latestData;
}