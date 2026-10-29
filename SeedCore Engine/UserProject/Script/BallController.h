#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class StopController;

class BallController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float respawnHeight = -10.0f;

private:
	SeedCore::Rigidbody* myRigid;
	SeedCore::SphereCollider* myCollider;
	SeedCore::LinearVelocity* velocity;

	StopController* stopController;

	bool isRespawn = false;

	float respawnDuration = 0.1f;
	float respawnTimer = 0.0f;

	SeedCore::Vector3 startPosition;
};
REGISTER_COMPONENT(BallController);
