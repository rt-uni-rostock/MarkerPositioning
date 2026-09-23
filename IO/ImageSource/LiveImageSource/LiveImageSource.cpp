#include "LiveImageSource.h"

#include "Logger.h"

namespace {
	// How long to wait before retrying to open/reopen the underlying video
	// stream after a failed attempt. Used both when the camera is not (yet)
	// reachable at all (e.g. missing at startup) and when a previously
	// working connection needs to be reestablished after a disconnect.
	constexpr std::chrono::milliseconds kReconnectRetryInterval{ 2000 };

	// How many consecutive empty/failed frame captures during normal
	// streaming are tolerated before treating the connection as lost and
	// closing/reopening the stream from scratch.
	constexpr int kConsecutiveFailuresBeforeReconnect = 3;

	// Poll granularity used while waiting for beginCapture() to be signaled,
	// and the chunk size for interruptibleSleep().
	constexpr std::chrono::milliseconds kPollInterval{ 100 };
}

LiveImageSource::LiveImageSource(std::unique_ptr<IVideoStream> stream) : stream_(std::move(stream))
{
	LOG_TRACE("LiveImageSource created with provided video stream");
}

LiveImageSource::~LiveImageSource()
{
	LOG_TRACE("LiveImageSource is being destroyed, stopping stream if running");

	if (running_) {
		LOG_TRACE("LiveImageSource is still running during destruction, stopping it now...");
		stop();
	}
}

void LiveImageSource::interruptibleSleep(std::chrono::milliseconds duration)
{
	auto remaining = duration;
	while (running_ && remaining.count() > 0) {
		auto chunk = std::min(remaining, kPollInterval);
		std::this_thread::sleep_for(chunk);
		remaining -= chunk;
	}
}

bool LiveImageSource::start()
{
	LOG_TRACE("Starting LiveImageSource...");
	if (running_) {
		LOG_WARN("LiveImageSource is already running, start() call ignored");
		return false; // already running
	}

	beginCaptureSignaled_ = false;
	opened_ = false;
	hasNewFrame_ = false;
	running_ = true;

	// The actual stream_->open() call happens asynchronously inside
	// lifecycleLoop(), so start() itself never fails just because the
	// camera is not currently reachable: it will simply keep retrying in
	// the background (see lifecycleLoop()). This lets the rest of the
	// application (e.g. a supervisor configured with multiple cameras)
	// start successfully even if some cameras are not yet connected.
	LOG_TRACE("Starting LiveImageSource lifecycle thread...");
	lifecycleThread_ = std::thread(&LiveImageSource::lifecycleLoop, this);

	return true;
}

bool LiveImageSource::beginCapture()
{
	LOG_TRACE("Beginning capture for LiveImageSource...");

	if (!running_) {
		LOG_ERROR("LiveImageSource::beginCapture() called before start(); ignoring.");
		return false;
	}

	beginCaptureSignaled_ = true;
	return true;
}

bool LiveImageSource::stop()
{
	LOG_TRACE("Stopping LiveImageSource...");

	if (!running_) {
		LOG_WARN("LiveImageSource is not running, stop() call ignored");
		return false; // already stopped
	}

	running_ = false;

	LOG_TRACE("Stopping LiveImageSource lifecycle thread...");
	if (lifecycleThread_.joinable()) {
		lifecycleThread_.join();
		LOG_TRACE("LiveImageSource lifecycle thread stopped successfully");
	}

	return true;
}

ImageFrame LiveImageSource::getLatestFrame()
{
	LOG_TRACE("Getting latest frame from LiveImageSource...");
	std::scoped_lock lock(frameMutex_);

	if (!running_) {
		LOG_ERROR("LiveImageSource is not running, cannot get latest frame");
		throw std::runtime_error("LiveImageSource is not running");
	}

	LOG_TRACE("Latest frame retrieved successfully");
	return latestFrame_;
}

bool LiveImageSource::hasNewFrame()
{
	// Atomically read-and-clear: the first caller after a new frame was
	// captured gets true, any subsequent caller gets false until the next
	// new frame arrives.
	return hasNewFrame_.exchange(false);
}

void LiveImageSource::lifecycleLoop()
{
	LOG_TRACE("Starting lifecycle loop in LiveImageSource...");

	// Calculate frame period based on config value maxCaptureFPS to control capture rate
	using clock = std::chrono::steady_clock;
	double maxFPS = stream_->getMaxCaptureFPS();
	const auto framePeriod = std::chrono::duration_cast<clock::duration>(
		std::chrono::duration<double>(1.0 / maxFPS));

	bool streaming = false;
	int consecutiveFailures = 0;

	while (running_) {

		// --- Ensure the stream is opened (device/control channel created).
		// If the camera is not (yet) reachable, this keeps retrying
		// indefinitely on a fixed backoff instead of failing permanently, so
		// a camera that is missing at startup or that disconnects mid-run
		// comes online automatically as soon as it becomes available. ---
		if (!opened_) {
			LOG_INFO("LiveImageSource attempting to open video stream...");
			if (stream_->open()) {
				opened_ = true;
				LOG_INFO("LiveImageSource successfully opened video stream.");
			}
			else {
				LOG_ERROR("LiveImageSource failed to open video stream (camera not connected/reachable); "
					"will keep retrying every {} ms.", kReconnectRetryInterval.count());
				interruptibleSleep(kReconnectRetryInterval);
				continue;
			}
		}

		// --- Wait until the supervisor has signaled it is fine to start
		// streaming (see IVideoStream::startStreaming() for why this
		// ordering matters for LUCID cameras). ---
		if (!beginCaptureSignaled_) {
			interruptibleSleep(kPollInterval);
			continue;
		}

		// --- Ensure image streaming has actually been started. ---
		if (!streaming) {
			if (!stream_->startStreaming()) {
				LOG_ERROR("LiveImageSource failed to start streaming; closing stream and will retry reconnecting...");
				stream_->close();
				opened_ = false;
				interruptibleSleep(kReconnectRetryInterval);
				continue;
			}
			streaming = true;
			consecutiveFailures = 0;
			LOG_INFO("LiveImageSource successfully started streaming.");
		}

		// --- Capture one frame. ---
		auto start = clock::now();

		ImageFrame frame;
		try {
			frame = stream_->getFrame();
		}
		catch (const std::exception& e) {
			// Defense in depth: a stream implementation should already catch
			// its own SDK-specific exceptions (e.g. LUCIDStream::getFrame()
			// does for Arena/GenICam exceptions), but an uncaught exception
			// escaping this thread would otherwise call std::terminate()/
			// abort() and crash the whole process, taking down every other
			// camera with it. Treat it like an empty frame instead.
			LOG_ERROR("Exception while retrieving frame in LiveImageSource lifecycle loop: {}", e.what());
		}

		if (frame.image.empty()) {
			++consecutiveFailures;
			LOG_WARN("Captured empty frame in LiveImageSource ({}/{} consecutive failures before reconnect)...",
				consecutiveFailures, kConsecutiveFailuresBeforeReconnect);

			if (consecutiveFailures >= kConsecutiveFailuresBeforeReconnect) {
				// The camera appears to have been disconnected (or otherwise
				// lost). Close the stream and go back to the top of the loop
				// to retry opening it from scratch; this recovers
				// automatically as soon as the camera reconnects, without
				// requiring an application restart.
				LOG_ERROR("LiveImageSource lost connection after {} consecutive failed frame captures; "
					"closing stream and attempting to reconnect...", consecutiveFailures);
				stream_->close();
				opened_ = false;
				streaming = false;
				consecutiveFailures = 0;
				continue;
			}

			// Sleep for the remaining frame period to control capture rate even when frames are empty
			auto elapsed = clock::now() - start;
			if (elapsed < framePeriod) {
				std::this_thread::sleep_for(framePeriod - elapsed);
			}
			continue;
		}

		consecutiveFailures = 0;

		{
			std::scoped_lock lock(frameMutex_);
			latestFrame_ = frame;
		}
		hasNewFrame_ = true;
		LOG_TRACE("New frame captured and stored in LiveImageSource");

		auto elapsed = clock::now() - start;
		if (elapsed < framePeriod) {
			std::this_thread::sleep_for(framePeriod - elapsed);
		}
	}

	// Ensure the stream is closed when the lifecycle loop exits (stop() was
	// called), regardless of which state it was in.
	if (opened_) {
		LOG_TRACE("LiveImageSource lifecycle loop exiting, closing video stream...");
		stream_->close();
		opened_ = false;
	}

	LOG_TRACE("LiveImageSource lifecycle loop finished.");
}
