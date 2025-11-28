// MarkerPositioning.cpp: Definiert den Einstiegspunkt für die Anwendung.
//

#include "MarkerPositioning.h"
#include "MainThreadsManager/MainThreadsManager.h"
#include "SettingsReader/SettingsReader.h"

using namespace std;

int main()
{
	SettingsReader settingsReader = SettingsReader("Settings.json");
	SettingsReader::MainSettings settings = settingsReader.settings;

	MainThreadsManager mainThreadsManager = MainThreadsManager(settings);

	return 0;

}
