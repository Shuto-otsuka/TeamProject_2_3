#include <FoundationEngine/World/Service.h>

namespace SeedCore
{
	Service::~Service()
	{
		for (auto it = slots_.rbegin(); it != slots_.rend(); ++it)
		{
			it->destroy_(it->service_);
		}
	}

	void* Service::Get(const String& type)const
	{
		for (const Slot& slot : slots_)
		{
			if (slot.type_ == type)
			{
				return slot.service_;
			}
		}
		return nullptr;
	}
}