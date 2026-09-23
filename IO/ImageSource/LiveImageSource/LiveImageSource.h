#pragma once

#include <thread>
#include <atomic>
#include <mutex>

#include "ImageSource/IImageSource.h"
#include "IVideoStream.h"

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class LiveImageSource : public IImageSource
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit LiveImageSource(std::unique_ptr<IVideoStream> stream);

	// Destructor: cleans up the main threads
	~LiveImageSource();

	bool start() override;
	bool stop() override;
	bool beginCapture() override;
	ImageFrame getLatestFrame() override;
	bool hasNewFrame() override;

private:
	// Runs for the entire lifetime between start() and stop(). Owns the full
	// connection lifecycle: opening the stream (retrying indefinitely with a
	// backoff if the camera is not yet reachable, e.g. not physically
	// connected at startup), waiting for beginCapture() to be signaled,
	// starting streaming, capturing frames, and — if the connection is lost
	// while running (repeated failed frame captures) — closing and
	// reopening the stream automatically to recover once the camera
	// reappears. This means a camera that is missing at startup or that
	// disconnects mid-run is handled identically: this loop just keeps
	// retrying until it succeeds again.
	void lifecycleLoop();

	// Sleeps for the given duration in small increments, checking running_
	// between each one, so stop() can interrupt a long backoff/poll wait
	// promptly instead of blocking for the full duration.
	void interruptibleSleep(std::chrono::milliseconds duration);

	std::unique_ptr<IVideoStream> stream_;

	std::thread lifecycleThread_;

	// True from start() until stop(); controls the lifetime of
	// lifecycleThread_. This is distinct from whether the underlying stream
	// is currently connected/streaming, which lifecycleLoop() tracks
	// internally and may toggle many times over the life of the thread.
	std::atomic<bool> running_ = false;

	// Set by beginCapture(); tells the lifecycle thread it may proceed to
	// call stream_->startStreaming() once the stream has been opened.
	std::atomic<bool> beginCaptureSignaled_ = false;

	// True while the stream is currently opened (device/control channel
	// created) and has not since been closed again. Used so the destructor
	// can detect (via the loop's own bookkeeping) whether a close() is
	// still owed; the lifecycle loop itself closes the stream on exit.
	std::atomic<bool> opened_ = false;

	// Set to true by lifecycleLoop() whenever a genuinely new (non-empty) frame
	// has been captured and stored in latestFrame_. hasNewFrame() atomically
	// consumes (resets) this flag so a frame is only ever reported as "new"
	// once, even if it is polled by multiple workers.
	std::atomic<bool> hasNewFrame_ = false;

	std::mutex frameMutex_;
	ImageFrame latestFrame_;
};