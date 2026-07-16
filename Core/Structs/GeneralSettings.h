#pragma once

#include <string>
#include <vector>

#include "SourceModeEnum.h"
#include "TagTypeEnum.h"
#include "CameraSettings.h"

// Image Logging Optionen
struct ImageLoggingOptions {
	bool saveRawFrames = false;			// Speichern von Originalframes
	bool saveGrayFrames = false;		// Speichern von Grayscale-Frames
	bool saveDetectionResults = false;	// Speichern von Frames nach Erkennung
	bool visualizeAllDetections = false;	// Visualisiere ALLE erkannten Tags, nicht nur den besten
};

// settings structure for file
struct GeneralSettings {
	SourceMode sourceMode = SourceMode::Live; 	// Modus der Bildquelle (0=Live, 1=Recorded)
	TagType tagType = TagType::AprilTag;		// Art des Fiducial Markers (0=AprilTag, 1=ArUco)
	// Tag Family?
	double tagSize = 0.1;						// Größe des AprilTags in Metern
	int tagID = 3;								// ID des zu erkennenden AprilTags
	double quadDecimate = 4.0;					// Quad-Decimation-Faktor für die Erkennung
	std::string udpIp = "192.168.3.50";			// Ziel-IP für UDP-Sende
	int udpPort = 5001;							// Ziel-Port für UDP-Sende
	double frameRate = 30.0;					// Frame-Rate der Kamera
	int streamId = 0;							// ID des Streams

	// Image Logging Konfiguration
	bool enableImageLogging = false;			// Master-Schalter für Image Logging
	std::string imageOutputPath = "./logs/frames/";	// Pfad, in dem Bilder gespeichert werden
	ImageLoggingOptions imageLogOptions;		// Optionen für Image Logging

	std::vector<CameraSettings> cameras;		// settings for multiple cameras, if needed in the future
};