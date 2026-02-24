#include "UdpPublisher.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // sorgt für Linken der Winsock-Bibliothek
#define CLOSE_SOCKET closesocket
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// RAII helper for Windows WSA initialization
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


// Constructor: creates a UDP socket and configures the target address and port for sending packets.
UdpPublisher::UdpPublisher(const std::string& address, uint16_t port) {

#ifdef _WIN32
	static WSAInitializer wsaInit; // initialized once
#endif

	initializeSocket(address, port);
}

// Destructor: closes the UDP socket to free system resources.
UdpPublisher::~UdpPublisher() {
	if (socket_ != INVALID_SOCKET)
	{
		CLOSE_SOCKET(socket_);
	}
}

void UdpPublisher::initializeSocket(const std::string& address, uint16_t port)
{
	socket_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_ == INVALID_SOCKET)
		throw std::runtime_error("Failed to create UDP socket");

	std::memset(&destAddr_, 0, sizeof(destAddr_));
	destAddr_.sin_family = AF_INET;
	destAddr_.sin_port = htons(port);

	if (inet_pton(AF_INET, address.c_str(), &destAddr_.sin_addr) <= 0)
		throw std::runtime_error("Invalid UDP address");
}

// Sends a PipelineResult by serializing it into a byte buffer and transmitting it via UDP to the configured address and port.
void UdpPublisher::send(const PipelineResult& result)
{
	std::array<uint8_t, UDP_PACKET_SIZE> buffer{};
	serialize(result, buffer);

	int sent = sendto(
		socket_,
		reinterpret_cast<const char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		0,
		reinterpret_cast<sockaddr*>(&destAddr_),
		sizeof(destAddr_));

	if (sent == SOCKET_ERROR)
	{
		// Do not throw here in real-time context unless required.
		std::cerr << "UDP send failed\n";
	}
}

// Serializes PipelineResult into fixed-size UDP packet.
// TODO: define byte layout specification
void UdpPublisher::serialize(const PipelineResult& r, std::array<uint8_t, UDP_PACKET_SIZE>& buffer) {
	uint8_t* ptr = buffer.data();

	auto write = [&](auto value)
		{
			std::memcpy(ptr, &value, sizeof(value));
			ptr += sizeof(value);
		};

	write(r.imageTimestamp);
	write(r.markerId);
	write(r.cameraId);
	write(r.markerType);
	write(r.errorCode);

	write(r.posX);
	write(r.posY);
	write(r.posZ);

	write(r.rotX);
	write(r.rotY);
	write(r.rotZ);

}