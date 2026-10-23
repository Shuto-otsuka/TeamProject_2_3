#include "CircuitSystem.h"
#include "SwitchSystem.h"

#include <SeedCore/ScComponent.h>
#include <SeedCore/ScLog.h>
#include <FoundationEngine/World/WorldAccessor.h>

using namespace SeedCore;

void CircuitSystem::OnStart()
{
	powered_ = false;
	latched_ = false;

	cacheActors_.clear();

	World& world = GetWorld();
	cacheActors_.reserve(inputActorIDs_.size());
	const Name* name = GetActor().GetComponent<Name>();
	for (Uint32 inputActorID : inputActorIDs_)
	{
		Actor findedActor = SeedCore::WorldAccessor::Find(world, inputActorID);
		if (findedActor)
		{
			cacheActors_.push_back(findedActor);
		}
		else
		{
			SC_LOG_WARNING("[{}] 入力 ID{} のアクターが見つかりません。無効なActorです。配線は有効ですか？", name ? name->name_.c_str() : "名前なし", inputActorID);
		}
	}
}

void CircuitSystem::OnLateTick(float elapsedTime)
{
	bool result = false;

	if (keepResult_ && latched_)
	{
		result = true;
	}

	int onCount = 0;
	for (const Actor& actor : cacheActors_)
	{
		SwitchSystem* switchSystem = actor.GetComponent<SwitchSystem>();
		if (!switchSystem || !actor.Active())
		{
			continue;
		}

		if (switchSystem->IsOn())
		{
			onCount++;
		}
	}

	switch (logic_)
	{
	case Logic::And:
		if (onCount > 0 && onCount == static_cast<int>(cacheActors_.size()))
		{
			result = true;
		}
		break;
	case Logic::Or:
		if (onCount >= 1)
		{
			result = true;
		}
		break;
	case Logic::Count:
		if (onCount >= guaranteeCount_)
		{
			result = true;
		}
		break;
	}

	if (keepResult_ && result)
	{
		latched_ = true;
	}

	if (flipResult_)
	{
		result = !result;
	}

	powered_ = result;
}

void CircuitSystem::OnDestroy()
{
	cacheActors_.clear();
}

bool CircuitSystem::IsPowered()const
{
	return powered_;
}