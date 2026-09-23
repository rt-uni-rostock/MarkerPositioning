#pragma once

#include "ImageSource/ImageFrame.h"
#include "ImageSource/ImageSourceConfig.h"

class IVideoStream
{
public:
	explicit IVideoStream(const ImageSourceConfig& config) : config_(config) {}

	virtual ~IVideoStream() = default;

	virtual bool open() = 0;
	virtual bool close() = 0;

	// Begins actual image streaming after open() has already created/connected
	// the device. Default no-op (true) for streams that already fully start
	// in open(). LUCID overrides this to defer Arena::IDevice::StartStream()
	// until all configured LUCID cameras have had their device created, to
	// avoid GVCP timeouts when opening one camera while another is already
	// streaming.
	virtual bool startStreaming() { return true; }

	virtual ImageFrame getFrame() = 0;

	const double getMaxCaptureFPS() const { return config_.maxCaptureFPS; }

protected:
	ImageSourceConfig config_;
};