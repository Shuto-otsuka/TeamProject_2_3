#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

class MoveStopController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		SeedCore::Vector3 moveDirection;		

private:
	float timeScale = 1.0f;
};
REGISTER_COMPONENT(MoveStopController);
