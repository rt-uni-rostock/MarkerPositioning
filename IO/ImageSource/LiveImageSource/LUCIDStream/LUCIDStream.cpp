#include "LUCIDStream.h"
#include "ImageSource/ImageFrame.h"
#include "LUCIDSystemManager.h"
#include "Logger.h"

#include <mutex>
#include <thread>
#include <chrono>

LUCIDStream::LUCIDStream(const ImageSourceConfig& config) : IVideoStream(config) {
	LOG_TRACE("LUCIDStream created with provided settings: mode={}, streamType={}",
		static_cast<int>(config_.mode), static_cast<int>(config_.cameraSettings->streamType));
}

LUCIDStream::~LUCIDStream() {

	LOG_TRACE("LUCIDStream is being destroyed, cleaning up resources...");

	if (system_ && device_)
	{
		std::scoped_lock lock(LUCIDSystemManager::mutex());
		try {
			system_->DestroyDevice(device_);
		}
		catch (const GenICam::GenericException& e) {
			LOG_ERROR("Exception while destroying LUCID device during shutdown: {}", e.GetDescription());
		}
		device_ = nullptr;
	}

	if (systemAcquired_)
	{
		LUCIDSystemManager::release();
		systemAcquired_ = false;
	}
	system_ = nullptr;
}

bool LUCIDStream::open() {

	LOG_TRACE("Opening LUCID camera stream...");

	try
	{
		// Arena only allows a single ISystem to be open per process, so the
		// system is shared across all LUCIDStream instances via
		// LUCIDSystemManager (see its header for details).
		system_ = LUCIDSystemManager::acquire();
		systemAcquired_ = true;

		if (!system_)
		{
			LOG_ERROR("Failed to acquire the shared Arena system (LUCIDSystemManager::acquire() returned null).");
			return false;
		}

		// serialize discovery/creation across all LUCID camera instances,
		// since they share the same underlying Arena system
		std::scoped_lock lock(LUCIDSystemManager::mutex());

		// get list of devices
		LOG_TRACE("Updating devices on system to discover connected cameras...");
		system_->UpdateDevices(100);
		std::vector<Arena::DeviceInfo> deviceInfos = system_->GetDevices();

		if (deviceInfos.size() == 0)
		{
			LOG_ERROR("No camera connected");
			return false;
		}

		LOG_TRACE("Number of LUCID cameras detected: {}", deviceInfos.size());
		for (auto& info : deviceInfos)
		{
			LOG_INFO("Detected LUCID camera: serial={}, model={}, ip={}",
				info.SerialNumber().c_str(), info.ModelName().c_str(), info.IpAddressStr().c_str());
		}

		const std::string& wantedSerial = config_.cameraSettings->serialNumber;

		Arena::DeviceInfo selectedDeviceInfo;
		bool foundDevice = false;

		if (!wantedSerial.empty())
		{
			// select the device whose serial number matches the configured camera
			LOG_TRACE("Selecting LUCID camera by configured serialNumber={}...", wantedSerial);
			for (auto& info : deviceInfos)
			{
				if (std::string(info.SerialNumber().c_str()) == wantedSerial)
				{
					selectedDeviceInfo = info;
					foundDevice = true;
					break;
				}
			}

			if (!foundDevice)
			{
				LOG_ERROR("No LUCID camera with serialNumber={} found among {} detected device(s). "
					"Check Settings.json cameras[].serialNumber against the detected serials logged above.",
					wantedSerial, deviceInfos.size());
				return false;
			}
		}
		else if (deviceInfos.size() == 1)
		{
			// backward-compatible fallback: single camera, no serialNumber configured
			LOG_WARN("No serialNumber configured for this LUCID camera; falling back to the single detected device "
				"(serial={}). Set 'serialNumber' in Settings.json to make this explicit.",
				deviceInfos[0].SerialNumber().c_str());
			selectedDeviceInfo = deviceInfos[0];
			foundDevice = true;
		}
		else
		{
			LOG_ERROR("Multiple LUCID cameras detected ({}) but no serialNumber configured for this camera entry. "
				"Set 'serialNumber' in Settings.json for each LUCID camera to select the correct physical device.",
				deviceInfos.size());
			return false;
		}

		LOG_TRACE("Creating device with serial={}...", selectedDeviceInfo.SerialNumber().c_str());

		// Opening a GVCP control channel for one camera can transiently time
		// out (GC_ERR_TIMEOUT) if another already-streaming LUCID camera is
		// simultaneously saturating the network with GVSP image data. This is
		// a real, observed condition when starting multiple LUCID cameras in
		// quick succession, so retry a few times with a short backoff before
		// giving up.
		constexpr int maxCreateDeviceAttempts = 5;
		constexpr auto createDeviceRetryDelay = std::chrono::milliseconds(750);
		device_ = nullptr;
		for (int attempt = 1; attempt <= maxCreateDeviceAttempts; ++attempt)
		{
			try
			{
				device_ = system_->CreateDevice(selectedDeviceInfo);
				break;
			}
			catch (const GenICam::GenericException& e)
			{
				if (attempt >= maxCreateDeviceAttempts)
				{
					throw;
				}
				LOG_WARN("Attempt {}/{} to create LUCID device (serial={}) failed with '{}', retrying in {} ms "
					"(likely transient GVCP contention with another camera being opened/streaming)...",
					attempt, maxCreateDeviceAttempts, selectedDeviceInfo.SerialNumber().c_str(), e.GetDescription(),
					createDeviceRetryDelay.count());
				std::this_thread::sleep_for(createDeviceRetryDelay);
			}
		}

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

		// StartStream() is intentionally NOT called here: it is deferred to
		// startStreaming(), which the caller (LiveImageSource/supervisor)
		// invokes only after ALL configured LUCID cameras have successfully
		// created their device. This avoids opening one camera's GVCP control
		// channel while another camera is already flooding the network with
		// GVSP image data, which reliably causes GC_ERR_TIMEOUT.
		return true;
	}
	catch (const GenICam::GenericException& e)
	{
		LOG_ERROR("Arena/GenICam exception while opening LUCID camera stream: {}", e.GetDescription());
		return false;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Exception while opening LUCID camera stream: {}", e.what());
		return false;
	}
}

bool LUCIDStream::startStreaming() {
	LOG_TRACE("Starting LUCID camera stream...");

	if (!device_)
	{
		LOG_ERROR("LUCIDStream::startStreaming() called without a created device.");
		return false;
	}

	try
	{
		std::scoped_lock lock(LUCIDSystemManager::mutex());
		device_->StartStream();
		return true;
	}
	catch (const GenICam::GenericException& e)
	{
		LOG_ERROR("Arena/GenICam exception while starting LUCID camera stream: {}", e.GetDescription());
		return false;
	}
}

bool LUCIDStream::close() {
	LOG_TRACE("Stopping LUCID camera stream...");

	try
	{
		if (system_ && device_) {
			std::scoped_lock lock(LUCIDSystemManager::mutex());
			system_->DestroyDevice(device_);
			device_ = nullptr;
		}
	}
	catch (const GenICam::GenericException& e)
	{
		LOG_ERROR("Arena/GenICam exception while closing LUCID camera stream: {}", e.GetDescription());
	}

	if (systemAcquired_)
	{
		LUCIDSystemManager::release();
		systemAcquired_ = false;
	}
	system_ = nullptr;

	return true;
}

ImageFrame LUCIDStream::getFrame() {
	// Implement the logic to retrieve the latest frame from the LUCID camera
	// This is a placeholder implementation and should be replaced with actual camera interaction code
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();

	// device_ can be null if the stream has been closed (e.g. by the
	// reconnect logic in LiveImageSource after a lost connection) but a
	// frame is still requested before the next open() has succeeded.
	// Guard against this explicitly: dereferencing a null device_ would be
	// undefined behavior (likely a crash), not a catchable GenICam/std
	// exception.
	if (!device_)
	{
		LOG_WARN("LUCIDStream::getFrame() called while the device is not open (camera disconnected/reconnecting); returning empty frame.");
		return latestData;
	}

	// All Arena/GenICam calls below can throw (e.g. GC_ERR_TIMEOUT if the
	// camera was physically disconnected or its network link dropped). Such
	// an exception previously escaped this function, propagated out of the
	// capture thread and caused std::terminate()/abort() to be called,
	// crashing the whole process. Catch it here instead: log the failure and
	// return an empty ImageFrame, which the caller (LiveImageSource capture
	// loop) already handles gracefully by skipping the cycle and retrying.
	Arena::IImage* image_ = nullptr;
	try
	{
		LOG_INFO("Retrieving latest frame from LUCID camera, frameId={}", frameCounter_ + 1);
		image_ = device_->GetImage(2000);

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
	}
	catch (const GenICam::GenericException& e)
	{
		LOG_ERROR("Arena/GenICam exception while retrieving frame from LUCID camera (camera may be disconnected or unreachable): {}", e.GetDescription());
		if (image_)
		{
			try { device_->RequeueBuffer(image_); }
			catch (const GenICam::GenericException&) { /* best-effort cleanup, ignore */ }
		}
		latestData.image = cv::Mat();
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Unexpected exception while retrieving frame from LUCID camera: {}", e.what());
		if (image_)
		{
			try { device_->RequeueBuffer(image_); }
			catch (const GenICam::GenericException&) { /* best-effort cleanup, ignore */ }
		}
		latestData.image = cv::Mat();
	}

	return latestData;
}