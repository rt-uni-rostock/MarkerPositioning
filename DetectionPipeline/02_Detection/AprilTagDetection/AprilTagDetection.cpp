#include "AprilTagDetection.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionResult.h"
#include "Logger.h"

AprilTagDetection::AprilTagDetection(const DetectionPipelineConfig& config) : config_(config) {
	LOG_TRACE("Initializing AprilTag Detection...");
	tf = tag16h5_create();
	td = apriltag_detector_create();
	apriltag_detector_add_family(td, tf);

	td->quad_decimate = config.quadDecimate;

	info.tagsize = config.tagSize; // Taggröße in Metern
	info.fx = config.fx; // Brennweite in Pixeln (x)
	info.fy = config.fy; // Brennweite in Pixeln (y)
	info.cx = config.cx; // Hauptpunkt x in Pixeln
	info.cy = config.cy; // Hauptpunkt y in Pixeln

	LOG_TRACE("Initialized AprilTag Detection with calibration parameters: fx={}, fy={}, cx={}, cy={}", info.fx, info.fy, info.cx, info.cy);
	LOG_TRACE("Also initialized tag size: {}.", info.tagsize);

	tagID = config.tagID;
}

AprilTagDetection::~AprilTagDetection() {
	LOG_TRACE("Destroying AprilTag Detection...");
	apriltag_detector_remove_family(td, tf);
	tag16h5_destroy(tf);
	apriltag_detector_destroy(td);
}

DetectionResult AprilTagDetection::process(const ImageFrame& frame) {

	LOG_TRACE("Processing frame with ID: {} in AprilTag Detection...", frame.frameId);

	DetectionResult result;

	if (frame.image.empty()) {
		LOG_ERROR("Frame with ID: {} is empty, skipping detection.", frame.frameId);
		result.success = false;
		result.message = "Empty frame, no detection performed.";
		return result;
	}

	LOG_TRACE("Converting frame with ID: {} to grayscale...", frame.frameId);

	cv::Mat gray;

	if (frame.image.channels() == 3) {
		LOG_TRACE("Frame with ID: {} has 3 channels, converting from BGR to grayscale...", frame.frameId);
		cv::cvtColor(frame.image, gray, cv::COLOR_BGR2GRAY);
	}
	else
		gray = frame.image;

	LOG_TRACE("Applying detection for frame with ID: {}...", frame.frameId);

	Pose pose = detect(gray);

	result.pose = pose;

	result.frameId = frame.frameId;
	result.timestamp = frame.timestamp;
	result.markerId = pose.tagId;
	result.success = pose.tagId != -1;

	LOG_TRACE("Finished processing frame with ID: {} in AprilTag Detection, success: {}, tag ID: {}, pose error: {}",
		frame.frameId, pose.tagId != -1, pose.tagId, pose.poseError);
	
	return result;
}

Pose AprilTagDetection::detect(const cv::Mat& gray) {

	LOG_TRACE("Starting AprilTag detection on grayscale image of size {}x{}...", gray.cols, gray.rows);

	// Prüfen, ob das Bild nicht leer ist
	if (gray.empty()) {
		LOG_ERROR("Grayscale image is empty, cannot perform detection.");
		return Pose{ 0,0,0,0,0,0 };
	}

	// Bild in apriltag_image_t umwandeln
	image_u8_t img_header = { gray.cols, gray.rows, gray.cols, gray.data };

	LOG_TRACE("Using apriltag_detector_detect to find tags in the image...");

	zarray_t* detections = apriltag_detector_detect(td, &img_header);

	apriltag_detection_t* det;

	LOG_TRACE("Selecting detection result for required tag ID: {}...", tagID);

	Pose pose = selectDetectionResult(detections, tagID, info);

	LOG_TRACE("Selected pose for tag ID {}: x={}, y={}, z={}, roll={}, pitch={}, yaw={}, pose error={}", pose.tagId, pose.x, pose.y, pose.z, pose.roll, pose.pitch, pose.yaw, pose.poseError);

	LOG_TRACE("Destroying detections array to free memory and returning pose.");

	apriltag_detections_destroy(detections);

	return pose;
}

Pose AprilTagDetection::selectDetectionResult(zarray_t* detections, int required_tag_id, apriltag_detection_info_t info) {

	LOG_TRACE("Selecting detection result. Number of detections: {}, required tag ID: {}...", zarray_size(detections), required_tag_id);

	AprilTagWithPose* selectedTag = nullptr;

	apriltag_detection_t* det;

	for (int i = 0; i < zarray_size(detections); i++) {

		zarray_get(detections, i, &det);

		if (det->id == required_tag_id) {

			LOG_TRACE("Found detection with required tag ID: {}, calculating pose...", det->id);

			info.det = det;

			apriltag_pose_t pose_result;
			double pose_err = estimate_tag_pose(&info, &pose_result);

			LOG_TRACE("Calculated pose for tag ID {}.", det->id);

			if (!selectedTag || pose_err < selectedTag->pose_err) {
				if (!selectedTag) {
					LOG_TRACE("Creating new selected tag for tag ID {} with pose error: {}...", det->id, pose_err);
					selectedTag = new AprilTagWithPose();
				} else
					LOG_TRACE("Found better pose for tag ID {} with lower error: {}, updating selected tag...", det->id, pose_err);
				selectedTag->detection = det;
				selectedTag->pose = pose_result;
				selectedTag->pose_err = pose_err;
			}

			//std::cout << "[AprilTagDetection] Pose Error for Tag ID " << det->id << ": " << pose_err << std::endl;
		}
	}

	if (!selectedTag || selectedTag->pose_err > 0.0001) {
		LOG_TRACE("No valid detection result found for required tag ID: {}, either no detection or pose error too high ({}), returning default pose.", required_tag_id, selectedTag ? selectedTag->pose_err : 0);
		return Pose{ 0,0,0,0,0,0 };
	}

	LOG_TRACE("Extracting pose for selected tag ID: {}, pose error: {}...", selectedTag->detection->id, selectedTag->pose_err);

	// Extrahiere Translation
	double x = selectedTag->pose.t->data[0]; // X-Translation
	double y = selectedTag->pose.t->data[1]; // Y-Translation
	double z = selectedTag->pose.t->data[2]; // Z-Translation

	// Extrahiere Rotation Matrix (pose_R) und konvertiere in Euler-Winkel
	double Rmat[3][3];
	for (int r = 0; r < 3; r++)
		for (int c = 0; c < 3; c++)
			Rmat[r][c] = selectedTag->pose.R->data[r * 3 + c];

	double roll = atan2(Rmat[2][1], Rmat[2][2]);
	double pitch = atan2(-Rmat[2][0], sqrt(Rmat[2][1] * Rmat[2][1] + Rmat[2][2] * Rmat[2][2]));
	double yaw = atan2(Rmat[1][0], Rmat[0][0]);

	LOG_TRACE("Returning pose for tag ID {}.", selectedTag->detection->id);

	// gebe die berechnete Pose zurück
	return Pose{ x,y,z, roll, pitch, yaw, required_tag_id, selectedTag->pose_err };
}
