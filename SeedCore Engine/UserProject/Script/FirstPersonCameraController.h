#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class FirstPersonCameraController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	void FollowPlayer(float elapsedTime);
	void Rotate(float elapsedTime);

	SC_REFLECTION_FIELD()
		SeedCore::Vector3 offset;
private:
	SeedCore::Actor player;
	SeedCore::Position* position;
	SeedCore::Rotation* rotation;
	SeedCore::Position* playerPosition;
	SeedCore::Rotation* playerRotation;
};
REGISTER_COMPONENT(FirstPersonCameraController);
