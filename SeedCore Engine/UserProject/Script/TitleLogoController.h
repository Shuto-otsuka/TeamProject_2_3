#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class TitleLogoController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float moveTime;
	SC_REFLECTION_FIELD()
		float minPosY;
	SC_REFLECTION_FIELD()
		float maxPosY;

private:
	SeedCore::Position* position;

	float rad = 0.0f;
};
REGISTER_COMPONENT(TitleLogoController);
