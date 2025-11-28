#include "apriltag.h"
#include "apriltag_pose.h"

struct AprilTagWithPose {
	apriltag_detection_t* detection;
	apriltag_pose_t pose;
	double pose_err;
};