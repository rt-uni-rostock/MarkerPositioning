#pragma once

struct Pose {
	double x;
	double y;
	double z;
	double roll;
	double pitch;
	double yaw;
	int tagId;
	double poseError;
};