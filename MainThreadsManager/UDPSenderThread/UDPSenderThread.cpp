#include "UDPSenderThread.h"
#include "PosePacket.h"
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
    PosePacket posePacket;
    posePacket.m1X = this->poseToSend.x;
    posePacket.m1Y = this->poseToSend.y;
    posePacket.m1Z = this->poseToSend.z;
	posePacket.m1Roll = this->poseToSend.roll;
	posePacket.m1Pitch = this->poseToSend.pitch;
	posePacket.m1Yaw = this->poseToSend.yaw;
	posePacket.m1TagId = this->poseToSend.tagId; // Example tag ID
	posePacket.m1PoseErr = this->poseToSend.poseError; // Example pose error
    posePacket.m2X = 0.0;
    posePacket.m2Y = 0;
    posePacket.m2Z = 0;
    posePacket.m2Roll = 0;
    posePacket.m2Pitch = 0;
    posePacket.m2Yaw = 0;
    posePacket.m2TagId = 0; // Example tag ID
    posePacket.m2PoseErr = 0.0; // Example pose error
    posePacket.m3X = 0;
    posePacket.m3Y = 0;
    posePacket.m3Z = 0;
    posePacket.m3Roll = 0;
    posePacket.m3Pitch = 0;
    posePacket.m3Yaw = 0;
    posePacket.m3TagId = 0; // Example tag ID
    posePacket.m3PoseErr = 0.0; // Example pose error

    std::cout << "Sending pose via UDP...\n";

    int sent = sendto(sock, reinterpret_cast<const char*>(&posePacket), sizeof(posePacket), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));
        
    if (sent == SOCKET_ERROR) {
        std::cerr << "sendto failed\n";
    }
    else {
        std::cout << "Pose sent: " << poseToSend.x << ", " << poseToSend.y << ", " << poseToSend.z
            << ", " << poseToSend.roll << ", " << poseToSend.pitch << ", " << poseToSend.yaw << "\n";
    }
}
