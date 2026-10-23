#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

class BulletNaviController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float interval;
	SC_REFLECTION_FIELD()
		float scrollSpeed;

private:
	SeedCore::Actor bulletStart;
	SeedCore::Actor bulletEnd;
	SeedCore::Actor line;

	SeedCore::Position* bulletStartPosition;
	SeedCore::Position* bulletEndPosition;

	SeedCore::Scale* bulletStartScale;
	SeedCore::Scale* bulletEndScale;

	SeedCore::Active* bulletStartActive;
	SeedCore::Active* bulletEndActive;

	PlayerController* playerController;

	float cubeStartInterval = 0.0f;
	float minSize;
	float maxSize;
};
REGISTER_COMPONENT(BulletNaviController);
