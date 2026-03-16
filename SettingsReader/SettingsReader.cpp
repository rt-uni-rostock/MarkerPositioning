#include "SettingsReader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "SourceModeEnum.h"
#include "StreamTypeEnum.h"
#include "TagTypeEnum.h"
#include "GeneralSettings.h"

using json = nlohmann::json;


/// <summary>
/// Convert CameraSettings to json, used for saving settings to file
/// </summary>
/// <param name="j">JSON Object</param>
/// <param name="c">Serialized CameraSettings object</param>
void to_json(json& j, const CameraSettings& c)
{
	j = json{
		{"id", c.id},
		{"streamType", static_cast<int>(c.streamType)},
		{"name", c.name},
		{"url", c.url},
		{"pipeline", c .pipeline},
		{"fx", c.fx},
		{"fy", c.fy},
		{"cx", c.cx},
		{"cy", c.cy},
		{"d1", c.d1},
		{"d2", c.d2},
		{"d3", c.d3},
		{"d4", c.d4},
		{"d5", c.d5},
		{"active", c.active}
	};
}

/// <summary>
/// deserializes json object to CameraSettings, used for loading settings from file
/// </summary>
/// <param name="j">JSON object, containing camera settings</param>
/// <param name="c">CameraSettings object</param>
void from_json(const json& j, CameraSettings& c)
{
	c.id = j.value("id", c.id);
	c.streamType = static_cast<StreamType>(j.value("streamType", static_cast<int>(c.streamType)));
	c.url = j.value("url", c.url);
	c.pipeline = j.value("pipeline", c.pipeline);
	c.fx = j.value("fx", c.fx);
	c.fy = j.value("fy", c.fy);
	c.cx = j.value("cx", c.cx);
	c.cy = j.value("cy", c.cy);
	c.d1 = j.value("d1", c.d1);
	c.d2 = j.value("d2", c.d2);
	c.d3 = j.value("d3", c.d3);
	c.d4 = j.value("d4", c.d4);
	c.d5 = j.value("d5", c.d5);
	c.active = j.value("active", c.active);
}

/// <summary>
/// Convert MainSettings to json, used for saving settings to file
/// </summary>
/// <param name="j">JSON Object</param>
/// <param name="s">Serialized MainSettings object</param>
void to_json(json& j, const GeneralSettings& s)
{
	j = json{
		{"sourceMode", static_cast<int>(s.sourceMode)},
		{"tagType", static_cast<int>(s.tagType)},
		{"tagSize", s.tagSize},
		{"tagID", s.tagID},
		{"quadDecimate", s.quadDecimate},
		{"udpIp", s.udpIp},
		{"udpPort", s.udpPort},
		{"frameRate", s.frameRate},
		{"cameras", s.cameras}
	};
}

void from_json(const json& j, GeneralSettings& s)
{
	// Defaults are already initialized in struct

	s.sourceMode = static_cast<SourceMode>(j.value("sourceMode", s.sourceMode));
	s.tagType = static_cast<TagType>(j.value("tagType", s.tagType));
	s.tagSize = j.value("tagSize", s.tagSize);
	s.tagID = j.value("tagID", s.tagID);

	s.quadDecimate = j.value("quadDecimate", s.quadDecimate);

	s.udpIp = j.value("udpIp", s.udpIp);
	s.udpPort = j.value("udpPort", s.udpPort);

	s.frameRate = j.value("frameRate", s.frameRate);

	if (j.contains("cameras"))
		s.cameras = j.at("cameras").get<std::vector<CameraSettings>>();
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
		std::cout << "Error loading settings: " << e.what() << ". Creating default settings file..." << std::endl;

		//writeDefaultSettings(filename);

		// Try again after creating defaults
		//LOG_TRACE("Attempting to load settings from file again: {}", filename);
		//settings_ = loadSettings(filename);
	}
}

const GeneralSettings& SettingsReader::get() const
{
	LOG_TRACE("Returning settings.");
	return settings_;
}

const std::vector<const CameraSettings*> SettingsReader::getActiveCameraSettingsList() const
{

	std::vector<const CameraSettings*> activeCameras;

	LOG_TRACE("Getting active camera settings from loaded settings.");
	for (const auto& cam : settings_.cameras) {
		if (cam.active) {
			LOG_TRACE("Active camera found: id={}, name={}", cam.id, cam.name);
			activeCameras.push_back(&cam);
		}
	}
	LOG_WARN("No active camera found in settings, returning default camera settings.");
	return activeCameras;
}

// load settings from file
GeneralSettings SettingsReader::loadSettings(const std::string& filename)
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
	return j.get<GeneralSettings>();
}

// write default settings to file
void SettingsReader::writeDefaultSettings(const std::string& filename)
{
	LOG_TRACE("Writing default settings to file: {}", filename);
	GeneralSettings defaultSettings;  // defaults from struct

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