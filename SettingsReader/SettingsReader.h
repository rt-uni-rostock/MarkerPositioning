#pragma once

#include <iostream>

class SettingsReader
{
public:
	struct MainSettings {
		int streamType;			// Art des Streams (0=RTSP, 1=LUCID)
		int tagType;			// Art des Fiducial Markers (0=AprilTag, 1=ArUco)
		// Tag Family?
		double tagSize;        // Größe des AprilTags in Metern
		int tagID;          // ID des zu erkennenden AprilTags
		double fx;            // Brennweite in Pixeln (x)
		double fy;            // Brennweite in Pixeln (y)
		double cx;            // Hauptpunkt x in Pixeln
		double cy;            // Hauptpunkt y in Pixeln
		double d1;            // Radiale Verzerrung k1
		double d2;            // Radiale Verzerrung k2
		double d3;            // Radiale Verzerrung k3
		double d4;            // Tangentiale Verzerrung p1
		double d5;            // Tangentiale Verzerrung p2
		double quadDecimate;  // Quad-Decimation-Faktor für die Erkennung
		std::string rtspUrl;   // RTSP URL der Kamera
		std::string udpIp;     // Ziel-IP für UDP-Sende
		int udpPort;          // Ziel-Port für UDP-Sende
		double frameRate;      // Frame-Rate der Kamera
	};
	SettingsReader(const std::string& filename);
	SettingsReader::MainSettings settings;
private:
	SettingsReader::MainSettings loadSettings(const std::string& filename);
	void writeDefaultSettings(const std::string& filename);
};
