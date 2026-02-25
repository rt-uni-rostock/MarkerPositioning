#pragma once

#include "02_Detection/IDetection.h"
#include "apriltag.h"
#include "apriltag_pose.h"
#include "tag16h5.h"
#include "02_Detection/DetectionConfig.h"
#include "02_Detection/Pose.h"
#include "02_Detection/AprilTagDetection/AprilTagWithPose.h"
#include <opencv2/opencv.hpp>

class AprilTagDetection : public IDetection {
public:
	AprilTagDetection(DetectionConfig config);
	~AprilTagDetection();

	Pose detect(const cv::Mat& image);

	int tagID;

private:
	apriltag_family_t* tf;
	apriltag_detector_t* td;
	apriltag_detection_info_t info;
	Pose selectDetectionResult(zarray_t* detections, int required_tag_id, apriltag_detection_info_t info);
};