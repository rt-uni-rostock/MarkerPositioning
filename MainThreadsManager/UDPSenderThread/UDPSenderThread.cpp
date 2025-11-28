#include "UDPSenderThread.h"
#ifdef _WIN32
#else
	#include <cstring>
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	typedef int SOCKET;
#endif

UDPSenderThread::UDPSenderThread(SettingsReader::MainSettings settings)
{
#ifdef _WIN32    
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
    }
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket creation failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(settings.udpPort);
    inet_pton(AF_INET, settings.udpIp.c_str(), &dest_addr.sin_addr);
}

UDPSenderThread::~UDPSenderThread()
{
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

void UDPSenderThread::startSending(Pose pose)
{
	this->poseToSend = pose;
    sendThread = std::jthread([this](std::stop_token st) {SendPoseThread(st); });
}

void UDPSenderThread::SendPoseThread(std::stop_token stopToken)
{
    std::cout << "Sending pose via UDP...\n";

    char buf[6 * sizeof(double)];
    memcpy(buf, &poseToSend.x, sizeof(double));
    memcpy(buf + 1 * sizeof(double), &poseToSend.y, sizeof(double));
    memcpy(buf + 2 * sizeof(double), &poseToSend.z, sizeof(double));
    memcpy(buf + 3 * sizeof(double), &poseToSend.roll, sizeof(double));
    memcpy(buf + 4 * sizeof(double), &poseToSend.pitch, sizeof(double));
    memcpy(buf + 5 * sizeof(double), &poseToSend.yaw, sizeof(double));

    int sent = sendto(sock, buf, sizeof(buf), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));

    if (sent == SOCKET_ERROR) {
        std::cerr << "sendto failed\n";
    }
    else {
        std::cout << "Pose sent: " << poseToSend.x << ", " << poseToSend.y << ", " << poseToSend.z
            << ", " << poseToSend.roll << ", " << poseToSend.pitch << ", " << poseToSend.yaw << "\n";
    }
}
