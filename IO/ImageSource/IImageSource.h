#pragma once

#include "ImageFrame.h"

class IImageSource
{
public:
	virtual ~IImageSource() = default;

	virtual bool start() = 0;
	virtual bool stop() = 0;
	virtual ImageFrame getLatestFrame() = 0;

	// Reports and consumes whether a genuinely new frame has been captured
	// since the last call to hasNewFrame(). Callers should only fetch (via
	// getLatestFrame()) and process a frame when this returns true; otherwise
	// no new data has arrived (e.g. camera temporarily disconnected, or the
	// capture rate is simply slower than the caller's poll rate) and the
	// previously delivered frame should not be logged/processed again.
	// Default implementation always reports a new frame, which is correct
	// for sources that synthesize/deliver a fresh frame on every call to
	// getLatestFrame() (e.g. StaticImageSource).
	virtual bool hasNewFrame() { return true; }

	// Optional second start-up phase: begins actual frame streaming/capture
	// after start() has already prepared (opened/connected) the source.
	// Splitting this out lets a supervisor open all sources first before
	// starting any of them to stream, which matters for LUCID GigE cameras
	// (only one device's control channel should be opened at a time while no
	// other device is already flooding the network with image data).
	// Default implementation is a no-op for sources that fully start in
	// start() already (e.g. StaticImageSource).
	virtual bool beginCapture() { return true; }
};