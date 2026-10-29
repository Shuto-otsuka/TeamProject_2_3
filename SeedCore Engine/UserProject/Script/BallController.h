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
	SC_REFLECTION_FIELD()
		float stopMass = 10.0f;

private:
	SeedCore::Rigidbody* myRigid;
	SeedCore::SphereCollider* myCollider;
	SeedCore::LinearVelocity* velocity;

	StopController* stopController;

	bool isRespawn = false;
	bool isStop = false;

	float respawnDuration = 0.1f;
	float respawnTimer = 0.0f;
	float noStopMass;

	SeedCore::Vector3 startPosition;
};
REGISTER_COMPONENT(BallController);
