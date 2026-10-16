#include "CircuitSystem.h"

#include <SeedCore/ScComponent.h>
#include <SeedCore/ScLog.h>

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
		Actor findedActor = world.FindActor(inputActorID);
		if (findedActor)
		{
			cacheActors_.push_back(findedActor);
		}
		else
		{
			SC_LOG_WARNING("[{}] 入力 ID{} のアクターが見つかりません。無効なActorです。配線は有効ですか？", name ? name->name_ : "名前なし", inputActorID);
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

	// TODO ループで回しボタンからOnかどうかをとる

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