// MarkerPositioning.cpp: entry point for the MarkerPositioning application.
//

#include "MarkerPositioning.h"
#include "MainThreadsManager/MainThreadsManager.h"
#include "SettingsReader/SettingsReader.h"

using namespace std;

int main()
{
	// Gstreamer evironment variables for debugging
    const char* gst_path = std::getenv("GST_PLUGIN_PATH");
    const char* path = std::getenv("PATH");

    if (gst_path)
        std::cout << "GST_PLUGIN_PATH = " << gst_path << std::endl;
    else
        std::cout << "GST_PLUGIN_PATH is not set" << std::endl;

    if (path)
        std::cout << "PATH = " << path << std::endl;
    else
        std::cout << "PATH is not set" << std::endl;

	// Load settings from JSON file
	SettingsReader settingsReader = SettingsReader("Settings.json");
	SettingsReader::MainSettings settings = settingsReader.settings;

	// Start main threads manager, which handles all threads, including image receiving, detection, and UDP sending
	MainThreadsManager mainThreadsManager = MainThreadsManager(settings);

	return 0;

}
