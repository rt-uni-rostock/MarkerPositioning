#pragma once
#include "ArenaApi.h"
#include <mutex>

// The Arena SDK explicitly documents that only one Arena::ISystem may be
// opened at a time per process (see Arena::OpenSystem() in Arena.h: "Only
// one system may be opened at a time."). Each LUCIDStream used to call
// Arena::OpenSystem()/CloseSystem() independently, which works for a single
// camera but crashes (or corrupts Arena's internal state) as soon as a
// second LUCIDStream tries to open its own system while another camera is
// already active.
//
// LUCIDSystemManager centralizes the system lifetime as a process-wide,
// reference-counted singleton so that multiple LUCIDStream instances (one
// per physical camera) can safely share the single allowed Arena::ISystem.
// It also exposes a mutex to serialize the system-wide Arena calls (device
// enumeration/creation/destruction) that operate on shared Arena state.
class LUCIDSystemManager {
public:
	// Acquires a reference to the shared system, opening it via
	// Arena::OpenSystem() if this is the first active reference. Must be
	// paired with a matching release() call.
	static Arena::ISystem* acquire();

	// Releases a reference previously obtained via acquire(). Closes the
	// shared system via Arena::CloseSystem() once the last reference is
	// released.
	static void release();

	// Mutex protecting system-wide Arena calls (UpdateDevices, GetDevices,
	// CreateDevice, DestroyDevice) that must not run concurrently from
	// multiple LUCIDStream instances.
	static std::mutex& mutex();

private:
	static std::mutex managerMutex_;
	static std::mutex callMutex_;
	static Arena::ISystem* system_;
	static int refCount_;
};
