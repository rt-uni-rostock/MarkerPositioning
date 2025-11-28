#include "../../SettingsReader/SettingsReader.h"
#include "../DetectionThread/Pose.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // sorgt für Linken der Winsock-Bibliothek
#define CLOSESOCKET closesocket
#define SOCKET_TYPE SOCKET
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define CLOSESOCKET close
#define SOCKET_TYPE int
#endif

#include <thread>

class UDPSenderThread {
public:
	UDPSenderThread(SettingsReader::MainSettings settings);
	~UDPSenderThread();
	void startSending(Pose pose);
private:
#ifdef _WIN32
	WSADATA wsa;
#endif
	SOCKET_TYPE sock;
	sockaddr_in dest_addr;
	Pose poseToSend;
	void SendPoseThread(std::stop_token stopToken);
	std::jthread sendThread;
};