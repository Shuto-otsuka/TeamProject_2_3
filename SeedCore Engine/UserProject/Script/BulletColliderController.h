#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class BulletController;

class BulletColliderController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerEnter(SeedCore::Actor other);

	BulletController* bulletController;

	SeedCore::Actor player;
};
REGISTER_COMPONENT(BulletColliderController);
