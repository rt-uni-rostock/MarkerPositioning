#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

#include "ImageSource/ImageFrame.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

class ImageUdpPublisher {
public:
	ImageUdpPublisher(
		const std::string& address,
		uint16_t port,
		std::size_t maxPayloadBytes = 1400,
		int pacingBurstChunks = 64,
		int pacingSleepMs = 1
	);
	~ImageUdpPublisher();

	void sendFrame(
		uint8_t cameraId,
		const ImageFrame& frame,
		std::chrono::system_clock::time_point receiveTimestamp
	);

private:
#pragma pack(push, 1)
	struct ImageChunkHeader {
		uint32_t magic = 0x4D50494D; // "MPIM"
		uint16_t version = 2;
		uint8_t byteOrder = 1; // 1 = little-endian
		uint8_t cameraId = 0;
		uint64_t frameId = 0;
		int64_t captureTimestampNs = 0;
		int64_t receiveTimestampNs = 0;
		int32_t width = 0;
		int32_t height = 0;
		int32_t cvType = 0;
		uint8_t channels = 0;
		uint8_t elemSizeBytes = 0;
		uint8_t depthCode = 0;
		uint8_t reserved0 = 0;
		uint32_t totalImageBytes = 0;
		uint32_t totalChunks = 0;
		uint32_t chunkIndex = 0;
		uint16_t chunkBytes = 0;
		uint16_t chunkStrideBytes = 0;
	};
#pragma pack(pop)

	void initializeSocket(const std::string& address, uint16_t port);
	static int64_t toEpochNanoseconds(std::chrono::system_clock::time_point tp);
	static void precisePace(int milliseconds);

#ifdef _WIN32
	using SocketType = SOCKET;
#else
	using SocketType = int;
#endif

	SocketType socket_;
	struct sockaddr_in destAddr_;
	std::size_t maxPayloadBytes_;
	int pacingBurstChunks_;
	int pacingSleepMs_;
};
