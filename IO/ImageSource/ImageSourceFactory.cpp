#include "ImageSourceFactory.h"

#include "SourceModeEnum.h"
#include "StreamTypeEnum.h"

#include "ImageSource/LiveImageSource/LiveImageSource.h"
#include "ImageSource/StaticImageSource/StaticImageSource.h"
#ifdef MP_ENABLE_LUCID
#include "ImageSource/LiveImageSource/LUCIDStream/LUCIDStream.h"
#endif
#include "ImageSource/LiveImageSource/RTPStream/RTPStream.h"
#include "ImageSource/LiveImageSource/RTSPStream/RTSPStream.h"
#include "ImageSource/LiveImageSource/WebcamStream/WebcamStream.h"

#include "Logger.h"


ImageSourceFactory::ImageSourceFactory(const ImageSourceConfig& config) : config_(config)
{
	LOG_TRACE("ImageSourceFactory initialized with config: mode={}, type={}, url={}",
		static_cast<int>(config_.mode),
		static_cast<int>(config_.cameraSettings->streamType),
		config_.cameraSettings->url);
}


std::unique_ptr<IImageSource> ImageSourceFactory::create()
{
	LOG_TRACE("Creating ImageSource...");

	if (config_.mode == SourceMode::Live) {

		LOG_TRACE("Live mode selected, creating video stream based on stream type: {}", static_cast<int>(config_.cameraSettings->streamType));

		std::unique_ptr<IVideoStream> stream;

		switch (config_.cameraSettings->streamType)
		{
		case StreamType::RTSP:
			stream = std::make_unique<RTSPStream>(config_);
			LOG_TRACE("RTSP stream created successfully.");
			break;
		case StreamType::RTP:
			stream = std::make_unique<RTPStream>(config_);
			LOG_TRACE("RTP stream created successfully.");
			break;
		case StreamType::LUCID:
#ifdef MP_ENABLE_LUCID
			stream = std::make_unique<LUCIDStream>(config_);
			LOG_TRACE("LUCID stream created successfully.");
#else
			LOG_ERROR("LUCID GigE camera support is not compiled into this build (ENABLE_LUCID=OFF).");
			throw std::runtime_error("LUCID GigE camera support is not available in this build (ENABLE_LUCID=OFF)");
#endif
			break;
		case StreamType::WEBCAM:
			stream = std::make_unique<WebcamStream>(config_);
			LOG_TRACE("Webcam stream created successfully.");
			break;
		default:
			LOG_ERROR("Unsupported stream type: {}", static_cast<int>(config_.cameraSettings->streamType));
			throw std::runtime_error("Unsupported stream type");
			break;
		}

		return std::make_unique<LiveImageSource>(std::move(stream));
	}
	else if (config_.mode == SourceMode::Recorded) {

		LOG_TRACE("Recorded mode selected.");

		return std::make_unique<StaticImageSource>();
	}
	else {

		LOG_ERROR("Unsupported source mode: {}", static_cast<int>(config_.mode));

		throw std::runtime_error("Unsupported source mode");
	}
}