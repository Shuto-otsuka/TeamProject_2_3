#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class BulletController;

class BulletColliderController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerEnter(SeedCore::Entity entity);

	BulletController* bulletController;

	SeedCore::Actor player;
};
REGISTER_COMPONENT(BulletColliderController);
