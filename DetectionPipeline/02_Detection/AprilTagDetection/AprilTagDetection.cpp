#include "AprilTagDetection.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionResult.h"
#include "Logger.h"
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

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
	result.frameId = frame.frameId;
	result.timestamp = frame.timestamp;

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

	// Image Logging: Speichern des Raw-Frames wenn aktiviert
	if (config_.enableImageLogging && config_.saveRawFrames) {
		saveImageToFile(frame.image, frame.frameId, "raw");
	}

	// Image Logging: Speichern des Grayscale-Frames wenn aktiviert
	if (config_.enableImageLogging && config_.saveGrayFrames) {
		saveImageToFile(gray, frame.frameId, "gray");
	}

	Pose pose = detect(gray);

	// Füge erkannten Marker zum Vektor hinzu
	if (pose.tagId != -1) {
		result.detectedMarkers.push_back(pose);
	}

	result.success = pose.tagId != -1;

	// Image Logging: Speichern des Frames mit Detection-Ergebnissen wenn aktiviert
	if (config_.enableImageLogging && config_.saveDetectionResults) {
		cv::Mat annotatedImage = frame.image.clone();
		
		if (config_.visualizeAllDetections) {
			// Führe eigenstände Erkennung durch um alle Tags zu visualisieren
			image_u8_t img_header = { gray.cols, gray.rows, gray.cols, gray.data };
			zarray_t* allDetections = apriltag_detector_detect(td, &img_header);
			drawAllDetections(annotatedImage, allDetections, pose.tagId);  // Übergebe ausgewählte Tag-ID
			apriltag_detections_destroy(allDetections);
		} else {
			// Zeichne nur den besten erkannten Tag
			if (result.success && pose.tagId != -1) {
				drawDetectionResult(annotatedImage, pose);
			}
		}
		
		saveImageToFile(annotatedImage, frame.frameId, "detection");
	}

	LOG_TRACE("Finished processing frame with ID: {} in AprilTag Detection, success: {}, tag ID: {}, pose error: {}",
		frame.frameId, pose.tagId != -1, pose.tagId, pose.poseError);
	
	return result;
}

Pose AprilTagDetection::detect(const cv::Mat& gray) {

	LOG_TRACE("Starting AprilTag detection on grayscale image of size {}x{}...", gray.cols, gray.rows);

	// Prüfen, ob das Bild nicht leer ist
	if (gray.empty()) {
		LOG_ERROR("Grayscale image is empty, cannot perform detection.");
		return Pose{ 0,0,0,0,0,0,-1 };
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

	bool found = false;
	AprilTagWithPose bestTag{};

	apriltag_detection_t* det;

	for (int i = 0; i < zarray_size(detections); i++) {

		zarray_get(detections, i, &det);

		if (det->id != required_tag_id)
			continue;

		LOG_TRACE("Found detection with required tag ID: {}, calculating pose...", det->id);

		info.det = det;

		apriltag_pose_t pose_result;
		double pose_err = estimate_tag_pose(&info, &pose_result);

		LOG_TRACE("Calculated pose for tag ID {}.", det->id);

		if (!found || pose_err < bestTag.pose_err) {
			if (found) {
				// free matrices from the previously best (now discarded) pose
				matd_destroy(bestTag.pose.R);
				matd_destroy(bestTag.pose.t);
			}
			LOG_TRACE("New best pose for tag ID {} with error: {}...", det->id, pose_err);
			bestTag.detection = det;
			bestTag.pose = pose_result;
			bestTag.pose_err = pose_err;
			found = true;
		} else {
			// discard this pose — free its matrices to avoid leak
			matd_destroy(pose_result.R);
			matd_destroy(pose_result.t);
		}
	}

	if (!found || bestTag.pose_err > 0.0001) {
		LOG_TRACE("No valid detection result found for required tag ID: {}, either no detection or pose error too high ({}), returning default pose.", required_tag_id, found ? bestTag.pose_err : 0.0);
		if (found) {
			matd_destroy(bestTag.pose.R);
			matd_destroy(bestTag.pose.t);
		}
		return Pose{ 0,0,0,0,0,0,-1 };
	}

	LOG_TRACE("Extracting pose for selected tag ID: {}, pose error: {}...", bestTag.detection->id, bestTag.pose_err);

	// Extrahiere Translation
	double x = bestTag.pose.t->data[0]; // X-Translation
	double y = bestTag.pose.t->data[1]; // Y-Translation
	double z = bestTag.pose.t->data[2]; // Z-Translation

	// Extrahiere Rotation Matrix (pose_R) und konvertiere in Euler-Winkel
	double Rmat[3][3];
	for (int r = 0; r < 3; r++)
		for (int c = 0; c < 3; c++)
			Rmat[r][c] = bestTag.pose.R->data[r * 3 + c];

	double roll = atan2(Rmat[2][1], Rmat[2][2]);
	double pitch = atan2(-Rmat[2][0], sqrt(Rmat[2][1] * Rmat[2][1] + Rmat[2][2] * Rmat[2][2]));
	double yaw = atan2(Rmat[1][0], Rmat[0][0]);

	LOG_TRACE("Returning pose for tag ID {}.", bestTag.detection->id);

	Pose result{ x, y, z, roll, pitch, yaw, required_tag_id, bestTag.pose_err };

	matd_destroy(bestTag.pose.R);
	matd_destroy(bestTag.pose.t);

	return result;
}

void AprilTagDetection::saveImageToFile(const cv::Mat& image, int frameId, const std::string& imageType) {
	try {
		// Erstelle den kompletten Pfad mit Kamera-ID
		std::string cameraDir = config_.imageOutputPath + "camera_" + std::to_string(config_.cameraId);
		fs::path outputDir(cameraDir);

		// Erstelle das Verzeichnis, falls es nicht existiert
		if (!fs::exists(outputDir)) {
			fs::create_directories(outputDir);
			LOG_TRACE("Created output directory for image logging: {}", cameraDir);
		}

		// Generiere einen eindeutigen Dateinamen mit Zeitstempel
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		std::stringstream filename;
		filename << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
		filename << "_" << std::setfill('0') << std::setw(3) << ms.count();
		filename << "_frame_" << frameId << "_" << imageType << ".png";

		std::string filePath = (outputDir / filename.str()).string();

		// Speichere das Bild
		if (cv::imwrite(filePath, image)) {
			LOG_TRACE("Saved {} image for frame {} to: {}", imageType, frameId, filePath);
		} else {
			LOG_ERROR("Failed to save {} image for frame {} to: {}", imageType, frameId, filePath);
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Exception while saving image: {}", e.what());
	}
}

void AprilTagDetection::drawDetectionResult(cv::Mat& image, const Pose& pose) {
	try {
		// Sicherheitsprüfungen: Nur zeichnen wenn die Erkennung valide ist
		if (pose.tagId == -1) {
			LOG_WARN("Cannot draw detection result: invalid tag ID");
			return;
		}

		// Prüfe ob die Pose-Werte sane sind
		if (pose.z <= 0 || std::isnan(pose.x) || std::isnan(pose.y) || std::isnan(pose.z) ||
			std::isnan(pose.roll) || std::isnan(pose.pitch) || std::isnan(pose.yaw)) {
			LOG_WARN("Cannot draw detection result: invalid pose values (z={}, x={}, y={})", pose.z, pose.x, pose.y);
			return;
		}

		// Achsenlänge skaliert zur Tag-Größe (0.5 = 50% der Tag-Höhe)
		double axisLength = info.tagsize * 0.5;

		// Rotationsmatrix aus Euler-Winkeln konstruieren (Roll-Pitch-Yaw)
		double cr = cos(pose.roll);
		double sr = sin(pose.roll);
		double cp = cos(pose.pitch);
		double sp = sin(pose.pitch);
		double cy = cos(pose.yaw);
		double sy = sin(pose.yaw);

		// Rotationsmatrix: R = Rz(yaw) * Ry(pitch) * Rx(roll)
		double R[3][3] = {
			{cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr},
			{sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr},
			{-sp,     cp * sr,                 cp * cr}
		};

		// Ursprung (Origin) des Tags projizieren
		double originX = info.fx * (pose.x / pose.z) + info.cx;
		double originY = info.fy * (pose.y / pose.z) + info.cy;

		// 3D Achsenpunkte im Tag-Koordinatensystem definieren
		double axisPointsLocal[3][3] = {
			{axisLength, 0,           0},           // X-Achse
			{0,          axisLength,  0},           // Y-Achse
			{0,          0,           axisLength}   // Z-Achse
		};

		// Achsenpunkte transformieren (rotieren und verschieben)
		double axisPointsGlobal[3][3];
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				axisPointsGlobal[i][j] = 0;
				for (int k = 0; k < 3; k++) {
					axisPointsGlobal[i][j] += R[j][k] * axisPointsLocal[i][k];
				}
				// Translation hinzufügen
				if (j == 0) axisPointsGlobal[i][j] += pose.x;
				else if (j == 1) axisPointsGlobal[i][j] += pose.y;
				else if (j == 2) axisPointsGlobal[i][j] += pose.z;
			}
		}

		// Projiziere die globalen 3D-Punkte auf die Bildebene
		auto projectPoint = [this](double x, double y, double z) -> cv::Point {
			if (z <= 0) return cv::Point(0, 0);
			double imgX = info.fx * (x / z) + info.cx;
			double imgY = info.fy * (y / z) + info.cy;
			return cv::Point(static_cast<int>(imgX), static_cast<int>(imgY));
		};

		cv::Point originImg = cv::Point(static_cast<int>(originX), static_cast<int>(originY));
		cv::Point xAxisImg = projectPoint(axisPointsGlobal[0][0], axisPointsGlobal[0][1], axisPointsGlobal[0][2]);
		cv::Point yAxisImg = projectPoint(axisPointsGlobal[1][0], axisPointsGlobal[1][1], axisPointsGlobal[1][2]);
		cv::Point zAxisImg = projectPoint(axisPointsGlobal[2][0], axisPointsGlobal[2][1], axisPointsGlobal[2][2]);

		// Origin als großer Punkt zeichnen
		cv::circle(image, originImg, 8, cv::Scalar(255, 255, 0), -1);
		cv::circle(image, originImg, 8, cv::Scalar(0, 0, 0), 2);

		// X-Achse (Rot) zeichnen
		cv::arrowedLine(image, originImg, xAxisImg, cv::Scalar(0, 0, 255), 2, 8, 0, 0.3);
		cv::putText(image, "X", cv::Point(xAxisImg.x + 5, xAxisImg.y - 5), 
			cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);

		// Y-Achse (Grün) zeichnen
		cv::arrowedLine(image, originImg, yAxisImg, cv::Scalar(0, 255, 0), 2, 8, 0, 0.3);
		cv::putText(image, "Y", cv::Point(yAxisImg.x - 15, yAxisImg.y - 5), 
			cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);

		// Z-Achse (Blau) zeichnen
		cv::arrowedLine(image, originImg, zAxisImg, cv::Scalar(255, 0, 0), 2, 8, 0, 0.3);
		cv::putText(image, "Z", cv::Point(zAxisImg.x + 5, zAxisImg.y + 15), 
			cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 0, 0), 2);

		// Text mit Tag-ID und Pose-Fehler neben dem Koordinatensystem platzieren
		std::string tagIDText = "ID: " + std::to_string(pose.tagId);
		
		// Formatiere Fehler als wissenschaftliche Notation
		std::stringstream errorStream;
		errorStream << std::scientific << std::setprecision(2) << pose.poseError;
		std::string errorText = "E:" + errorStream.str();
		
		// Definiere die Textfarbe basierend auf dem Fehler
		cv::Scalar color = getColorByError(pose.poseError);
		
		cv::putText(image, tagIDText, cv::Point(originImg.x + 15, originImg.y - 10), 
			cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
		cv::putText(image, errorText, cv::Point(originImg.x + 15, originImg.y + 5), 
			cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

		LOG_TRACE("Drawn coordinate system for tag ID: {} with error: {}", pose.tagId, pose.poseError);
	} catch (const std::exception& e) {
		LOG_ERROR("Exception while drawing detection result: {}", e.what());
	}
}

void AprilTagDetection::drawAllDetections(cv::Mat& image, zarray_t* detections, int selected_tag_id) {
	try {
		if (detections == nullptr || zarray_size(detections) == 0) {
			LOG_TRACE("No detections to draw");
			return;
		}

		LOG_TRACE("Drawing all {} detections - selected tag ID: {}", zarray_size(detections), selected_tag_id);

		apriltag_detection_t* det;

		for (int i = 0; i < zarray_size(detections); i++) {
			zarray_get(detections, i, &det);

			// Berechne Pose für diesen Tag
			info.det = det;
			apriltag_pose_t pose_result;
			double pose_err = estimate_tag_pose(&info, &pose_result);

			// Erstelle Pose-Struktur
			double x = pose_result.t->data[0];
			double y = pose_result.t->data[1];
			double z = pose_result.t->data[2];

			double Rmat[3][3];
			for (int r = 0; r < 3; r++)
				for (int c = 0; c < 3; c++)
				 Rmat[r][c] = pose_result.R->data[r * 3 + c];

			double roll = atan2(Rmat[2][1], Rmat[2][2]);
			double pitch = atan2(-Rmat[2][0], sqrt(Rmat[2][1] * Rmat[2][1] + Rmat[2][2] * Rmat[2][2]));
			double yaw = atan2(Rmat[1][0], Rmat[0][0]);

			Pose pose = { x, y, z, roll, pitch, yaw, det->id, pose_err };

			// Prüfe ob Pose valide ist
			if (pose.z <= 0 || std::isnan(pose.x) || std::isnan(pose.y) || std::isnan(pose.z) ||
				std::isnan(pose.roll) || std::isnan(pose.pitch) || std::isnan(pose.yaw)) {
				LOG_WARN("Invalid pose for tag ID {}, skipping visualization", det->id);
				continue;
			}

			// Farbe basierend auf ob es der ausgewählte Tag ist
			cv::Scalar color;
			if (det->id == selected_tag_id) {
				// Ausgewählter Tag: GRÜN
				color = cv::Scalar(0, 255, 0);  // BGR: Grün
				LOG_TRACE("Tag ID {} is selected - drawing in GREEN", det->id);
			} else {
				// Andere Tags: ORANGE
				color = cv::Scalar(0, 165, 255);  // BGR: Orange
			}

			// Achsenlänge skaliert zur Tag-Größe
			double axisLength = info.tagsize * 0.5;

			// Rotationsmatrix aus Euler-Winkeln konstruieren
			double cr = cos(pose.roll);
			double sr = sin(pose.roll);
			double cp = cos(pose.pitch);
			double sp = sin(pose.pitch);
			double cy = cos(pose.yaw);
			double sy = sin(pose.yaw);

			// Rotationsmatrix: R = Rz(yaw) * Ry(pitch) * Rx(roll)
			double R[3][3] = {
				{cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr},
				{sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr},
				{-sp,     cp * sr,                 cp * cr}
			};

			// Ursprung (Origin) des Tags projizieren
			double originX = info.fx * (pose.x / pose.z) + info.cx;
			double originY = info.fy * (pose.y / pose.z) + info.cy;
			cv::Point originImg = cv::Point(static_cast<int>(originX), static_cast<int>(originY));

			// 3D Achsenpunkte im Tag-Koordinatensystem definieren
			double axisPointsLocal[3][3] = {
				{axisLength, 0,           0},           // X-Achse
				{0,          axisLength,  0},           // Y-Achse
				{0,          0,           axisLength}   // Z-Achse
			};

			// Achsenpunkte transformieren (rotieren und verschieben)
			double axisPointsGlobal[3][3];
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 3; k++) {
					axisPointsGlobal[j][k] = 0;
					for (int l = 0; l < 3; l++) {
						axisPointsGlobal[j][k] += R[k][l] * axisPointsLocal[j][l];
					}
					// Translation hinzufügen
					if (k == 0) axisPointsGlobal[j][k] += pose.x;
					else if (k == 1) axisPointsGlobal[j][k] += pose.y;
					else if (k == 2) axisPointsGlobal[j][k] += pose.z;
				}
			}

			// Projiziere die globalen 3D-Punkte auf die Bildebene
			auto projectPoint = [this](double x, double y, double z) -> cv::Point {
				if (z <= 0) return cv::Point(0, 0);
				double imgX = info.fx * (x / z) + info.cx;
				double imgY = info.fy * (y / z) + info.cy;
				return cv::Point(static_cast<int>(imgX), static_cast<int>(imgY));
			};

			cv::Point xAxisImg = projectPoint(axisPointsGlobal[0][0], axisPointsGlobal[0][1], axisPointsGlobal[0][2]);
			cv::Point yAxisImg = projectPoint(axisPointsGlobal[1][0], axisPointsGlobal[1][1], axisPointsGlobal[1][2]);
			cv::Point zAxisImg = projectPoint(axisPointsGlobal[2][0], axisPointsGlobal[2][1], axisPointsGlobal[2][2]);

			// Origin mit Farbe zeichnen
			cv::circle(image, originImg, 6, color, -1);
			cv::circle(image, originImg, 6, cv::Scalar(0, 0, 0), 1);

			// X-Achse zeichnen
			cv::arrowedLine(image, originImg, xAxisImg, color, 1, 8, 0, 0.25);
			cv::putText(image, "X", cv::Point(xAxisImg.x + 3, xAxisImg.y - 3), 
				cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

			// Y-Achse zeichnen
			cv::arrowedLine(image, originImg, yAxisImg, color, 1, 8, 0, 0.25);
			cv::putText(image, "Y", cv::Point(yAxisImg.x - 10, yAxisImg.y - 3), 
				cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

			// Z-Achse zeichnen
			cv::arrowedLine(image, originImg, zAxisImg, color, 1, 8, 0, 0.25);
			cv::putText(image, "Z", cv::Point(zAxisImg.x + 3, zAxisImg.y + 10), 
				cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

			// Text mit Tag-ID und Pose-Fehler neben dem Koordinatensystem platzieren
			std::string tagIDText = "ID:" + std::to_string(pose.tagId);

			// Formatiere Fehler als wissenschaftliche Notation
			std::stringstream errorStream;
			errorStream << std::scientific << std::setprecision(2) << pose_err;
			std::string errorText = "E:" + errorStream.str();

			cv::putText(image, tagIDText, cv::Point(originImg.x + 15, originImg.y - 10),
				cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
			cv::putText(image, errorText, cv::Point(originImg.x + 15, originImg.y + 5),
				cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);

			LOG_TRACE("Drawn coordinate system for tag ID: {} with error: {}", det->id, pose_err);
		}

	} catch (const std::exception& e) {
		LOG_ERROR("Exception while drawing all detections: {}", e.what());
	}
}

cv::Scalar AprilTagDetection::getColorByError(double error, double maxError) {
	// Normalisiere den Fehler auf [0, 1] range
	// 0 = guter Fehler (Grün), 1 = schlechter Fehler (Rot)
	double normalizedError = std::min(error / maxError, 1.0);
	
	// Farbverlauf: Grün -> Gelb -> Orange -> Rot
	// HSV wird zu BGR konvertiert
	
	cv::Mat hsv(1, 1, CV_8UC3);
	
	// Hue: Von 60 (Grün) zu 0 (Rot) - je schlechter desto rötlicher
	int hue = static_cast<int>(60 * (1.0 - normalizedError));
	
	// Saturation: 255 (bei guten Fehlern gesättigt) bis 150 (bei schlechten weniger gesättigt)
	int saturation = static_cast<int>(255 * (1.0 - normalizedError * 0.4));
	
	// Value: 255 (hell und sichtbar)
	int value = 255;
	
	hsv.at<cv::Vec3b>(0, 0) = cv::Vec3b(hue, saturation, value);
	
	cv::Mat bgr;
	cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
	
	cv::Vec3b bgrColor = bgr.at<cv::Vec3b>(0, 0);
	
	return cv::Scalar(bgrColor[0], bgrColor[1], bgrColor[2]);
}
