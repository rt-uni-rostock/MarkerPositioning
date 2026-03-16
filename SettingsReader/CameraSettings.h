#pragma once

#include <string>
#include "StreamTypeEnum.h"

// settings structure for file
struct CameraSettings {
	int id = 0;						// unique camera id
	
	StreamType streamType = StreamType::WEBCAM; // type of stream (0=NONE, 1=LUCID, 2=RTP, 3=RTSP, 4=Webcam)
	std::string name = "";						// human-readable name for the camera, used for logging and debugging
	
	std::string url = "";						// for stream: RTSP/RTP URL, for recorded mode: path to image files
	std::string pipeline = "";					// for stream: gstreamer pipeline string

	double fx = 1.0;							// Brennweite in Pixeln (x)
	double fy = 1.0;							// Brennweite in Pixeln (y)
	double cx = 1.0;							// Hauptpunkt x in Pixeln
	double cy = 1.0;							// Hauptpunkt y in Pixeln
	double d1 = 0.0;							// Radiale Verzerrung k1
	double d2 = 0.0;							// Radiale Verzerrung k2
	double d3 = 0.0;							// Radiale Verzerrung k3
	double d4 = 0.0;							// Tangentiale Verzerrung p1
	double d5 = 0.0;							// Tangentiale Verzerrung p2

	bool active = false;						// whether this camera should be used in the application
};