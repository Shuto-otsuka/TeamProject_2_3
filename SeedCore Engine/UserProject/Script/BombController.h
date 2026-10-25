#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class StopController;

class BombController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnEditorTick(float elapsedTime);
	void UpdateMoveTarget();

	SC_REFLECTION_FIELD()
		float moveX;
	SC_REFLECTION_FIELD()
		float moveZ;
	SC_REFLECTION_FIELD()
		float moveSpeed;
	SC_REFLECTION_FIELD()
		float turnSpeed;
	SC_REFLECTION_FIELD()
		float explosionInterval;
	SC_REFLECTION_FIELD()
		float explosionRadius;
	SC_REFLECTION_FIELD()
		bool startX = true;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 timerOffset = {0,0,0};

private:
	DirectX::XMFLOAT3 targetPoints[4];
	int startNum = 0;
	int endNum = 1;

	float moveRate = 0.0f;
	float explosionTimer = 0.0f;

	SeedCore::Actor bombTimer;
	SeedCore::Text* bombTimerText;
	SeedCore::Position* bombTimerPosition;
	SeedCore::Rigidbody* myRigid;
	SeedCore::Rotation* rotation;
	SeedCore::LinearVelocity* linearVelocity;
	SeedCore::AngularVelocity* angularVelocity;
	StopController* stopController;
};
REGISTER_COMPONENT(BombController);
