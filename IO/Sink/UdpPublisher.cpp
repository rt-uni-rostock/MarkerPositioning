#include "UdpPublisher.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

#include "Logger.h"

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

	if (inet_pton(AF_INET, address.c_str(), &destAddr_.sin_addr) <= 0) {
		CLOSE_SOCKET(socket_);
		socket_ = INVALID_SOCKET;
		throw std::runtime_error("Invalid UDP address");
	}
}

// Sends a MarkerMessage by serializing it into a byte buffer and transmitting it via UDP to the configured address and port.
void UdpPublisher::send(const MarkerMessage& message)
{
	std::array<uint8_t, UDP_PACKET_SIZE> buffer{};
	serialize(message, buffer);

	int sent = sendto(
		socket_,
		reinterpret_cast<const char*>(buffer.data()),
		static_cast<int>(UDP_PACKET_SIZE),
		0,
		reinterpret_cast<sockaddr*>(&destAddr_),
		sizeof(destAddr_));

	if (sent == SOCKET_ERROR)
	{
		LOG_ERROR("UDP send failed for marker ID: {} (camera ID: {})", message.markerId, message.cameraId);
	}
	else
	{
		LOG_TRACE("UDP message sent for marker ID: {} (camera ID: {}, size: {} bytes)", message.markerId, message.cameraId, sent);
	}
}

// Serializes MarkerMessage into fixed-size UDP packet.
void UdpPublisher::serialize(const MarkerMessage& msg, std::array<uint8_t, UDP_PACKET_SIZE>& buffer) {
	uint8_t* ptr = buffer.data();

	auto write = [&](auto value)
	{
		std::memcpy(ptr, &value, sizeof(value));
		ptr += sizeof(value);
	};

	// Serialize rotation
	write(static_cast<double>(msg.rotX));
	write(static_cast<double>(msg.rotZ));
	write(static_cast<double>(msg.rotY));

	// Reserved field
	write(static_cast<double>(-1));

	// Serialize position
	write(static_cast<double>(msg.posX));
	write(static_cast<double>(msg.posY));
	write(static_cast<double>(msg.posZ));

	// Marker ID
	write(static_cast<uint8_t>(msg.markerId));

	// Kamera-ID
	write(static_cast<double>(msg.cameraId));

	// Remaining fields can be extended as needed
	double padding = 0.0;
	for (int i = 0; i < 11; ++i) {
		write(padding);
	}
}