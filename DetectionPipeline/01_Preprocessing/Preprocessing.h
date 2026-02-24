#pragma once

// The Supervisor class manages the whole detection pipeline. All pipeline steps are configured and if needed threads created.
class Preprocessing
{
public:
	// Constructor: initializes the main threads with the given settings
	Preprocessing();

	// Destructor: cleans up the main threads
	~Preprocessing();
};