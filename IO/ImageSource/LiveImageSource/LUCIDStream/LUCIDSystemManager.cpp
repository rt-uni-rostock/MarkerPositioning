#include "LUCIDSystemManager.h"
#include "Logger.h"

std::mutex LUCIDSystemManager::managerMutex_;
std::mutex LUCIDSystemManager::callMutex_;
Arena::ISystem* LUCIDSystemManager::system_ = nullptr;
int LUCIDSystemManager::refCount_ = 0;

Arena::ISystem* LUCIDSystemManager::acquire()
{
	std::scoped_lock lock(managerMutex_);

	if (refCount_ == 0)
	{
		LOG_TRACE("LUCIDSystemManager: opening the shared Arena system (first active LUCID camera)...");
		system_ = Arena::OpenSystem();
	}

	++refCount_;
	LOG_TRACE("LUCIDSystemManager: acquired shared Arena system, active references: {}", refCount_);
	return system_;
}

void LUCIDSystemManager::release()
{
	std::scoped_lock lock(managerMutex_);

	if (refCount_ <= 0)
	{
		LOG_WARN("LUCIDSystemManager: release() called without a matching acquire(), ignoring.");
		return;
	}

	--refCount_;
	LOG_TRACE("LUCIDSystemManager: released shared Arena system, active references: {}", refCount_);

	if (refCount_ == 0 && system_)
	{
		LOG_TRACE("LUCIDSystemManager: closing the shared Arena system (last active LUCID camera released it)...");
		Arena::CloseSystem(system_);
		system_ = nullptr;
	}
}

std::mutex& LUCIDSystemManager::mutex()
{
	return callMutex_;
}
