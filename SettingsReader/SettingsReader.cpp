#include "SettingsReader.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

SettingsReader::SettingsReader(const std::string& filename)
{
	try {
		settings = SettingsReader::loadSettings(filename);
	}
	catch (const std::exception& e) {
		// create file with default settings
		std::cerr << "Creating default settings" << std::endl;
		SettingsReader::writeDefaultSettings(filename);
	}
}

SettingsReader::MainSettings SettingsReader::loadSettings(const std::string& filename)
{
	std::ifstream f(filename);
	if (!f.is_open()) {
		std::cerr << "Error: Could not open file " << filename << std::endl;
		throw std::runtime_error("Could not open config file");
	}

	json j;
	f >> j;

	SettingsReader::MainSettings cfg;
	cfg.streamType = j.value("streamType", 0);
	cfg.tagType = j.value("tagType", 0);
	cfg.tagSize = j.value("tagSize", 0.1);
	cfg.tagID = j.value("tagID", 1);
	cfg.fx = j.value("fx", 1);
	cfg.fy = j.value("fy", 1);
	cfg.cx = j.value("cx", 1);
	cfg.cy = j.value("cy", 1);
	cfg.d1 = j.value("d1", 0.0);
	cfg.d2 = j.value("d2", 0.0);
	cfg.d3 = j.value("d3", 0.0);
	cfg.d4 = j.value("d4", 0.0);
	cfg.d5 = j.value("d5", 0.0);
	cfg.quadDecimate = j.value("quadDecimate", 4.0);
	cfg.rtspUrl = j.value("rtspUrl", "rtsp://localhost:58000/live");
	cfg.udpIp = j.value("udpIp", "192.168.3.50");
	cfg.udpPort = j.value("udpPort", 5001);
	cfg.frameRate = j.value("frameRate", 30.0);
	return cfg;
}

void SettingsReader::writeDefaultSettings(const std::string& filename)
{
	SettingsReader::MainSettings defaultSettings;
	defaultSettings.streamType = 0;
	defaultSettings.tagType = 0;
	defaultSettings.tagSize = 0.1;
	defaultSettings.tagID = 3;
	defaultSettings.fx = 1;
	defaultSettings.fy = 1;
	defaultSettings.cx = 1;
	defaultSettings.cy = 1;
	defaultSettings.d1 = 0.0;
	defaultSettings.d2 = 0.0;
	defaultSettings.d3 = 0.0;
	defaultSettings.d4 = 0.0;
	defaultSettings.d5 = 0.0;
	defaultSettings.quadDecimate = 4.0;
	defaultSettings.rtspUrl = "rtsp://localhost:58000/live";
	defaultSettings.udpIp = "192.168.3.50";
	defaultSettings.udpPort = 5001;
	defaultSettings.frameRate = 30.0;

	json j;
	j["tagSize"] = defaultSettings.tagSize;
	j["tagID"] = defaultSettings.tagID;
	j["fx"] = defaultSettings.fx;
	j["fy"] = defaultSettings.fy;
	j["cx"] = defaultSettings.cx;
	j["cy"] = defaultSettings.cy;
	j["d1"] = defaultSettings.d1;
	j["d2"] = defaultSettings.d2;
	j["d3"] = defaultSettings.d3;
	j["d4"] = defaultSettings.d4;
	j["d5"] = defaultSettings.d5;
	j["quadDecimate"] = defaultSettings.quadDecimate;
	j["rtspUrl"] = defaultSettings.rtspUrl;
	j["udpIp"] = defaultSettings.udpIp;
	j["udpPort"] = defaultSettings.udpPort;
	j["frameRate"] = defaultSettings.frameRate;
	std::ofstream o(filename);
	if (!o.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
		return;
	}
	o << j.dump(4);
}