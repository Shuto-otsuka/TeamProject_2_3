#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class GunController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

private:
	SeedCore::Actor player;
};
REGISTER_COMPONENT(GunController);
