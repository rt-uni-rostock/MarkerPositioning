#pragma once



#include "IImageSource.h"

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class StaticImageSource : public IImageSource
{
public:
	// Constructor: initializes the main threads with the given settings
	explicit StaticImageSource();

	// Destructor: cleans up the main threads
	~StaticImageSource();

	void start() override;
	void stop() override;

	ImageFrame getLatestFrame() override;

private:
	size_t index_ = 0;
	bool running_ = false;
};