#include "AprilTagDetection.h"

AprilTagDetection::AprilTagDetection(SettingsReader::MainSettings settings) {
	tf = tag16h5_create();
	td = apriltag_detector_create();
	apriltag_detector_add_family(td, tf);
	td->quad_decimate = settings.quadDecimate;

	info.tagsize = settings.tagSize; // Taggröße in Metern
	info.fx = settings.fx; // Brennweite in Pixeln (x)
	info.fy = settings.fy; // Brennweite in Pixeln (y)
	info.cx = settings.cx; // Hauptpunkt x in Pixeln
	info.cy = settings.cy; // Hauptpunkt y in Pixeln

	// log calibration parameters
	std::cout << "[AprilTagDetection] Initialized with calibration parameters:\n";
	std::cout << "  fx: " << info.fx << "\n";
	std::cout << "  fy: " << info.fy << "\n";
	std::cout << "  cx: " << info.cx << "\n";
	std::cout << "  cy: " << info.cy << "\n";

	tagID = settings.tagID;
}

AprilTagDetection::~AprilTagDetection() {
	apriltag_detector_remove_family(td, tf);
	tag16h5_destroy(tf);
	apriltag_detector_destroy(td);
}

Pose AprilTagDetection::detect(const cv::Mat& gray) {

	// Prüfen, ob das Bild nicht leer ist
	if (gray.empty()) {
		std::cerr << "[AprilTagDetection] Image is empty." << std::endl;
		return Pose{ 0,0,0,0,0,0 };
	}

	// Bild in apriltag_image_t umwandeln
	image_u8_t img_header = { gray.cols, gray.rows, gray.cols, gray.data };

	zarray_t* detections = apriltag_detector_detect(td, &img_header);

	apriltag_detection_t* det;

	Pose pose = selectDetectionResult(detections, tagID, info);

	apriltag_detections_destroy(detections);

	return pose;
}

Pose AprilTagDetection::selectDetectionResult(zarray_t* detections, int required_tag_id, apriltag_detection_info_t info) {

	AprilTagWithPose* selectedTag = nullptr;

	apriltag_detection_t* det;

	for (int i = 0; i < zarray_size(detections); i++) {

		zarray_get(detections, i, &det);

		if (det->id == required_tag_id) {

			info.det = det;

			apriltag_pose_t pose_result;
			double pose_err = estimate_tag_pose(&info, &pose_result);

			if (!selectedTag || pose_err < selectedTag->pose_err) {
				if (!selectedTag) {
					selectedTag = new AprilTagWithPose();
				}
				selectedTag->detection = det;
				selectedTag->pose = pose_result;
				selectedTag->pose_err = pose_err;
			}

			//std::cout << "[AprilTagDetection] Pose Error for Tag ID " << det->id << ": " << pose_err << std::endl;
		}
	}

	if (!selectedTag || selectedTag->pose_err > 0.0001) {
		std::cout << "[AprilTagDetection] no valid detection result\n";
		return Pose{ 0,0,0,0,0,0 };
	}

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

	// gebe die berechnete Pose zurück
	return Pose{ x,y,z, roll, pitch, yaw, required_tag_id, 0 };
}
