#include "ImageUdpPublisher.h"

#include <bit>
#include <chrono>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#include "Logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#ifdef _WIN32
class WSAInitializer {
public:
	WSAInitializer() {
		if (WSAStartup(MAKEWORD(2, 2), &wsa_) != 0) {
			throw std::runtime_error("WSAStartup failed");
		}
	}

	~WSAInitializer() {
		WSACleanup();
	}
private:
	WSADATA wsa_;
};
#endif

ImageUdpPublisher::ImageUdpPublisher(
	const std::string& address,
	uint16_t port,
	std::size_t maxPayloadBytes,
	int pacingBurstChunks,
	int pacingSleepMs
)
	: maxPayloadBytes_(maxPayloadBytes)
	, pacingBurstChunks_(pacingBurstChunks > 0 ? pacingBurstChunks : 8)
	, pacingSleepMs_(pacingSleepMs >= 0 ? pacingSleepMs : 1) {
#ifdef _WIN32
	static WSAInitializer wsaInit;
#endif

	if (std::endian::native != std::endian::little) {
		throw std::runtime_error("ImageUdpPublisher supports little-endian hosts only.");
	}

	if (maxPayloadBytes_ < sizeof(ImageChunkHeader)) {
		throw std::runtime_error("Image UDP max payload must be at least as large as the chunk header.");
	}

	const std::size_t maxChunkData = maxPayloadBytes_ - sizeof(ImageChunkHeader);
	if (maxChunkData > static_cast<std::size_t>((std::numeric_limits<uint16_t>::max)())) {
		throw std::runtime_error("Image UDP max payload is too large for chunkBytes metadata field.");
	}

	initializeSocket(address, port);
}

ImageUdpPublisher::~ImageUdpPublisher() {
	if (socket_ != INVALID_SOCKET) {
		CLOSE_SOCKET(socket_);
	}
}

void ImageUdpPublisher::initializeSocket(const std::string& address, uint16_t port) {
	socket_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_ == INVALID_SOCKET) {
		throw std::runtime_error("Failed to create UDP socket for image publisher");
	}

	const int sendBufferSize = 8 * 1024 * 1024;
	setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendBufferSize), sizeof(sendBufferSize));

	std::memset(&destAddr_, 0, sizeof(destAddr_));
	destAddr_.sin_family = AF_INET;
	destAddr_.sin_port = htons(port);

	if (inet_pton(AF_INET, address.c_str(), &destAddr_.sin_addr) <= 0) {
		CLOSE_SOCKET(socket_);
		socket_ = INVALID_SOCKET;
		throw std::runtime_error("Invalid UDP address for image publisher");
	}
}

int64_t ImageUdpPublisher::toEpochNanoseconds(std::chrono::system_clock::time_point tp) {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
}

void ImageUdpPublisher::precisePace(int milliseconds) {
	if (milliseconds <= 0) {
		return;
	}

	// Sleep()/sleep_for() alone round up to the Windows scheduler tick
	// (~15.6ms by default), so a "1ms" pacing pause was silently costing
	// ~15ms in practice - about 15x more than intended. At ~70 pacing
	// pauses per 1080p frame that alone added roughly a second of latency.
	// To get real millisecond-level precision without changing the
	// system-wide timer resolution (timeBeginPeriod), sleep coarsely for
	// most of the requested duration, leaving a safety margin, then
	// spin-wait the remainder using a high-resolution clock.
	using clock = std::chrono::steady_clock;
	const auto target = clock::now() + std::chrono::milliseconds(milliseconds);

	constexpr int coarseSleepMarginMs = 2;
	if (milliseconds > coarseSleepMarginMs) {
#ifdef _WIN32
		Sleep(static_cast<DWORD>(milliseconds - coarseSleepMarginMs));
#else
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds - coarseSleepMarginMs));
#endif
	}

	while (clock::now() < target) {
		std::this_thread::yield();
	}
}

void ImageUdpPublisher::sendFrame(
	uint8_t cameraId,
	const ImageFrame& frame,
	std::chrono::system_clock::time_point receiveTimestamp
) {
	if (frame.image.empty()) {
		return;
	}

	const cv::Mat contiguousImage = frame.image.isContinuous() ? frame.image : frame.image.clone();
	const std::size_t totalBytes = contiguousImage.total() * contiguousImage.elemSize();

	const std::size_t headerSize = sizeof(ImageChunkHeader);
	if (maxPayloadBytes_ <= headerSize) {
		throw std::runtime_error("Image UDP max payload is too small for chunk header");
	}
	const std::size_t maxChunkDataBytes = maxPayloadBytes_ - headerSize;

	const std::size_t chunkCount = (totalBytes + maxChunkDataBytes - 1) / maxChunkDataBytes;
	if (chunkCount > static_cast<std::size_t>((std::numeric_limits<uint32_t>::max)())) {
		throw std::runtime_error("Image is too large for UDP chunk metadata");
	}
	if (totalBytes > static_cast<std::size_t>((std::numeric_limits<uint32_t>::max)())) {
		throw std::runtime_error("Image is too large for UDP frame metadata");
	}

	const auto* imageData = contiguousImage.ptr<uint8_t>(0);
	const int64_t captureTs = toEpochNanoseconds(frame.timestamp);
	const int64_t receiveTs = toEpochNanoseconds(receiveTimestamp);

	for (std::size_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
		const std::size_t offset = chunkIndex * maxChunkDataBytes;
		const std::size_t remaining = totalBytes - offset;
		const std::size_t currentChunkBytes = (remaining > maxChunkDataBytes) ? maxChunkDataBytes : remaining;

		ImageChunkHeader header;
		header.cameraId = cameraId;
		header.frameId = frame.frameId;
		header.captureTimestampNs = captureTs;
		header.receiveTimestampNs = receiveTs;
		header.width = contiguousImage.cols;
		header.height = contiguousImage.rows;
		header.cvType = contiguousImage.type();
		header.channels = static_cast<uint8_t>(contiguousImage.channels());
		header.elemSizeBytes = static_cast<uint8_t>(contiguousImage.elemSize1());
		header.depthCode = static_cast<uint8_t>(contiguousImage.depth());
		header.totalImageBytes = static_cast<uint32_t>(totalBytes);
		header.totalChunks = static_cast<uint32_t>(chunkCount);
		header.chunkIndex = static_cast<uint32_t>(chunkIndex);
		header.chunkBytes = static_cast<uint16_t>(currentChunkBytes);
		header.chunkStrideBytes = static_cast<uint16_t>(maxChunkDataBytes);

		std::vector<uint8_t> packet(headerSize + currentChunkBytes);
		std::memcpy(packet.data(), &header, headerSize);
		std::memcpy(packet.data() + headerSize, imageData + offset, currentChunkBytes);

		const int sent = sendto(
			socket_,
			reinterpret_cast<const char*>(packet.data()),
			static_cast<int>(packet.size()),
			0,
			reinterpret_cast<sockaddr*>(&destAddr_),
			sizeof(destAddr_));

		if (sent == SOCKET_ERROR) {
			throw std::runtime_error("UDP image chunk send failed");
		}

		// Pace the burst so the receiver's OS socket buffer is not overrun.
		// Uses precisePace() (sleep + spin-wait) instead of a plain
		// Sleep()/sleep_for() call - see precisePace() for why that matters.
		// This deliberately trades a little latency for much better delivery
		// reliability, especially important for large (e.g. 1080p) frames
		// that are split into thousands of chunks.
		const bool isLastChunk = (chunkIndex + 1 == chunkCount);
		if (!isLastChunk && pacingSleepMs_ > 0 &&
			(static_cast<int>(chunkIndex + 1) % pacingBurstChunks_) == 0) {
			precisePace(pacingSleepMs_);
		}
	}
}
