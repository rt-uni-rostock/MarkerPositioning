#include <stdint.h>

#pragma pack(push, 1)
struct PosePacket
{
	double m1Roll, m1Yaw, m1Pitch;
	double m1PoseErr;
	double m1X, m1Y, m1Z;
	uint8_t m1TagId;
	double m2Roll, m2Yaw, m2Pitch;
	double m2PoseErr;
	double m2X, m2Y, m2Z;
	uint8_t m2TagId;
	double m3Roll, m3Yaw, m3Pitch;
	double m3PoseErr;
	double m3X, m3Y, m3Z;
	uint8_t m3TagId;

};
#pragma pack(pop)

// das struct ist so gepackt, dass es in der Visualisierung in der richtigen Reihenfolge ankommt