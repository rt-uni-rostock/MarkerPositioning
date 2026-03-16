#pragma once

#include <iostream>
#include "GeneralSettings.h"
#include "CameraSettings.h"

// read settings from a configuration file
class SettingsReader
{
public:
	// constructor
	explicit SettingsReader(const std::string& filename);

	const GeneralSettings& get() const;

	const std::vector<const CameraSettings*> getActiveCameraSettingsList() const;
private:
	GeneralSettings settings_;

	// load settings from file
	GeneralSettings loadSettings(const std::string& filename);

	// write default settings to file
	void writeDefaultSettings(const std::string& filename);
};
