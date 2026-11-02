#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

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
	PlayerController* playerController;

	float pitch = 0.0f;
	float yaw = 0.0f;
};
REGISTER_COMPONENT(FirstPersonCameraController);
