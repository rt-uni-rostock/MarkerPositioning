#include "SettingsReader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "ImageSource/SourceModeEnum.h"
#include "ImageSource/StreamTypeEnum.h"
#include "02_Detection/TagTypeEnum.h"

using json = nlohmann::json;


void to_json(json& j, const MainSettings& s)
{
	j = json{
		{"sourceMode", static_cast<int>(s.sourceMode)},
		{"streamType", static_cast<int>(s.streamType)},
		{"tagType", static_cast<int>(s.tagType)},
		{"tagSize", s.tagSize},
		{"tagID", s.tagID},
		{"fx", s.fx},
		{"fy", s.fy},
		{"cx", s.cx},
		{"cy", s.cy},
		{"d1", s.d1},
		{"d2", s.d2},
		{"d3", s.d3},
		{"d4", s.d4},
		{"d5", s.d5},
		{"quadDecimate", s.quadDecimate},
		{"rtspUrl", s.rtspUrl},
		{"udpIp", s.udpIp},
		{"udpPort", s.udpPort},
		{"frameRate", s.frameRate}
	};
}

void from_json(const json& j, MainSettings& s)
{
	// Defaults are already initialized in struct

	s.sourceMode = static_cast<SourceMode>(j.value("sourceMode", s.sourceMode));
	s.streamType = static_cast<StreamType>(j.value("streamType", s.streamType));
	s.tagType = static_cast<TagType>(j.value("tagType", s.tagType));
	s.tagSize = j.value("tagSize", s.tagSize);
	s.tagID = j.value("tagID", s.tagID);

	s.fx = j.value("fx", s.fx);
	s.fy = j.value("fy", s.fy);
	s.cx = j.value("cx", s.cx);
	s.cy = j.value("cy", s.cy);

	s.d1 = j.value("d1", s.d1);
	s.d2 = j.value("d2", s.d2);
	s.d3 = j.value("d3", s.d3);
	s.d4 = j.value("d4", s.d4);
	s.d5 = j.value("d5", s.d5);

	s.quadDecimate = j.value("quadDecimate", s.quadDecimate);

	s.rtspUrl = j.value("rtspUrl", s.rtspUrl);
	s.udpIp = j.value("udpIp", s.udpIp);
	s.udpPort = j.value("udpPort", s.udpPort);

	s.frameRate = j.value("frameRate", s.frameRate);
}



// constructor, tries to load settings from file, if fails creates default settings file
SettingsReader::SettingsReader(const std::string& filename)
{
	try {
		LOG_TRACE("Attempting to load settings from file: {}", filename);
		settings_ = loadSettings(filename);
	}
	catch (const std::exception& e) {
		LOG_ERROR("Error loading settings: {}. Creating default settings file...", e.what());

		writeDefaultSettings(filename);

		// Try again after creating defaults
		LOG_TRACE("Attempting to load settings from file again: {}", filename);
		settings_ = loadSettings(filename);
	}
}

const MainSettings& SettingsReader::get() const
{
	LOG_TRACE("Returning settings.");
	return settings_;
}

// load settings from file
MainSettings SettingsReader::loadSettings(const std::string& filename)
{
	LOG_TRACE("Loading settings from file.");
	// try to open file
	std::ifstream f(filename);
	if (!f.is_open()) {
		LOG_ERROR("Could not open config file: {}. Throwing runtime error.", filename);
		throw std::runtime_error("Could not open config file");
	}

	// parse json
	json j;
	f >> j;

	LOG_TRACE("Settings loaded from file: {}", filename);
	return j.get<MainSettings>();
}

// write default settings to file
void SettingsReader::writeDefaultSettings(const std::string& filename)
{
	LOG_TRACE("Writing default settings to file: {}", filename);
	MainSettings defaultSettings;  // defaults from struct

	json j = defaultSettings;

	LOG_TRACE("Default settings JSON: {}", j.dump(4));
	std::ofstream file(filename);
	if (!file.is_open())
	{
		LOG_ERROR("Could not open default settings file for writing: {}. Throwing runtime error.", filename);
		throw std::runtime_error("Could not open config file for writing");
	}

	file << j.dump(4);
}