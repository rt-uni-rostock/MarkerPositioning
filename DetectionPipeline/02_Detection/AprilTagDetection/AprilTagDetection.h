#pragma once

#include "02_Detection/IDetection.h"
#include "apriltag.h"
#include "apriltag_pose.h"
#include "tag16h5.h"
#include "DetectionPipelineConfig.h"
#include "02_Detection/Pose.h"
#include "02_Detection/AprilTagDetection/AprilTagWithPose.h"
#include <opencv2/opencv.hpp>

class AprilTagDetection : public IDetection {
public:
	explicit AprilTagDetection(const DetectionPipelineConfig& config);
	
	~AprilTagDetection();

	DetectionResult process(const ImageFrame& frame) override;

	int tagID;

private:
	DetectionPipelineConfig config_;

	apriltag_family_t* tf;
	apriltag_detector_t* td;
	apriltag_detection_info_t info;
	
	Pose detect(const cv::Mat& image);
	Pose selectDetectionResult(zarray_t* detections, int required_tag_id, apriltag_detection_info_t info);
	
	// Hilfsfunktionen zum Speichern und Annotieren von Bildern
	void saveImageToFile(const cv::Mat& image, int frameId, const std::string& imageType);
	void drawDetectionResult(cv::Mat& image, const Pose& pose);
	void drawAllDetections(cv::Mat& image, zarray_t* detections, int selected_tag_id);
	Pose detectionToPose(apriltag_detection_t* det);
	
	// Hilfsfunktion für fehlerbasierte Farbzuordnung
	cv::Scalar getColorByError(double error, double maxError = 0.1);
};