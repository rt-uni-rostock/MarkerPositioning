#include <stdint.h>

#pragma pack(push, 1)
struct PosePacket
{
	double m1Roll, m1Pitch, m1Yaw;
	double m1PoseErr;
	double m1X, m1Y, m1Z;
	uint8_t m1TagId;
	double m2Roll, m2Pitch, m2Yaw;
	double m2PoseErr;
	double m2X, m2Y, m2Z;
	uint8_t m2TagId;
	double m3Roll, m3Pitch, m3Yaw;
	double m3PoseErr;
	double m3X, m3Y, m3Z;
	uint8_t m3TagId;

};
#pragma pack(pop)