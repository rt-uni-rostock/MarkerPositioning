#pragma once

#include <string>

#include "ImageSource/SourceModeEnum.h"
#include "ImageSource/StreamTypeEnum.h"
#include "02_Detection/TagTypeEnum.h"

// settings structure for file
struct MainSettings {
	SourceMode sourceMode = SourceMode::Live; 				// Modus der Bildquelle (0=Live, 1=Recorded)
	StreamType streamType = StreamType::RTP;				// Art des Streams (0=NONE, 1=LUCID, 2=RTP, 3=RTSP)
	TagType tagType = TagType::AprilTag;					// Art des Fiducial Markers (0=AprilTag, 1=ArUco)
	// Tag Family?
	double tagSize = 0.1;									// Größe des AprilTags in Metern
	int tagID = 3;											// ID des zu erkennenden AprilTags
	double fx = 1.0;										// Brennweite in Pixeln (x)
	double fy = 1.0;										// Brennweite in Pixeln (y)
	double cx = 1.0;										// Hauptpunkt x in Pixeln
	double cy = 1.0;										// Hauptpunkt y in Pixeln
	double d1 = 0.0;										// Radiale Verzerrung k1
	double d2 = 0.0;										// Radiale Verzerrung k2
	double d3 = 0.0;										// Radiale Verzerrung k3
	double d4 = 0.0;										// Tangentiale Verzerrung p1
	double d5 = 0.0;										// Tangentiale Verzerrung p2
	double quadDecimate = 4.0;								// Quad-Decimation-Faktor für die Erkennung
	std::string rtspUrl = "rtsp://localhost:58000/live";    // RTSP URL der Kamera
	std::string udpIp = "192.168.3.50";						// Ziel-IP für UDP-Sende
	int udpPort = 5001;										// Ziel-Port für UDP-Sende
	double frameRate = 30.0;								// Frame-Rate der Kamera
};