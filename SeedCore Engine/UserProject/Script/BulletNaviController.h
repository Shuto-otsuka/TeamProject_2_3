#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

class BulletNaviController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float minSize;
	SC_REFLECTION_FIELD()
		float maxSize;

private:
	SeedCore::Actor bulletStart;
	SeedCore::Actor bulletEnd;

	SeedCore::Position* bulletStartPosition;
	SeedCore::Position* bulletEndPosition;

	SeedCore::Scale* bulletStartScale;
	SeedCore::Scale* bulletEndScale;

	SeedCore::Active* bulletStartActive;
	SeedCore::Active* bulletEndActive;

	PlayerController* playerController;
};
REGISTER_COMPONENT(BulletNaviController);
