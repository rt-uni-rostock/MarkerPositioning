#pragma once

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class PostProcessing
{
public:
	// Constructor: initializes the main threads with the given settings
	PostProcessing();

	// Destructor: cleans up the main threads
	~PostProcessing();

};