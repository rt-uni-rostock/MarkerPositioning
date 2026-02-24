#pragma once

#include <iostream>
#include "MainSettings.h"

// read settings from a configuration file
class SettingsReader
{
public:
	// constructor
	explicit SettingsReader(const std::string& filename);

	const MainSettings& get() const;
private:
	MainSettings settings_;

	// load settings from file
	MainSettings loadSettings(const std::string& filename);

	// write default settings to file
	void writeDefaultSettings(const std::string& filename);
};
