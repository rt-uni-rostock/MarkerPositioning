#pragma once
#include "PipelineResult.h"
#include <array>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

// handles UDP socket creation and packet transmission.
// this class does not contain any threading logic.

class UdpPublisher {
public:
	// Constructor and destructor
	UdpPublisher(const std::string& address, uint16_t port);
	~UdpPublisher();

	// sends PipelineResult as serialized byte buffer via UDP to configured address and port
	void send(const PipelineResult& result);

private:

	static constexpr size_t UDP_PACKET_SIZE =
		sizeof(std::string) +
		4 * sizeof(int32_t) +
		6 * sizeof(float);

	// Serializes PipelineResult into fixed-size UDP packet.
	void serialize(const PipelineResult& result, std::array<uint8_t, UDP_PACKET_SIZE>& buffer);

	void initializeSocket(const std::string& address, uint16_t port);

#ifdef _WIN32
	using SocketType = SOCKET;
#else
	using SocketType = int;
#endif

	// UDP socket file descriptor and target address structure
	SocketType socket_;
	struct sockaddr_in destAddr_;

};