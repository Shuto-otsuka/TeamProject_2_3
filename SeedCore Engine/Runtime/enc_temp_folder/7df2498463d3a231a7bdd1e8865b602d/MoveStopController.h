#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

class MoveStopController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
};
REGISTER_COMPONENT(MoveStopController);
