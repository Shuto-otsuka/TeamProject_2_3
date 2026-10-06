#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

class BulletController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void SetParam(const SeedCore::Vector3& startPosition,const SeedCore::Vector3& moveDirection, float chargeRate,int cost);

	void OnCollisionEnter(SeedCore::Entity entity);
	void Stop();
	void Remove();

	bool IsStop() const { return isStop; }

	float GetAliveTimer() const { return aliveTimer; }

	SC_REFLECTION_FIELD()
		float minSize;
	SC_REFLECTION_FIELD()
		float maxSize;
	SC_REFLECTION_FIELD()
		float minSpeed;
	SC_REFLECTION_FIELD()
		float maxSpeed;
	SC_REFLECTION_FIELD()
		float minAliveTime;
	SC_REFLECTION_FIELD()
		float maxAliveTime;
	SC_REFLECTION_FIELD()
		float minTurnSpeed;
	SC_REFLECTION_FIELD()
		float maxTurnSpeed;
	SC_REFLECTION_FIELD()
		float removeDistance;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 turnDirection;

private:
	void UpdateRemove(float elapsedTime);
	void UpdateAliveTime(float elapsedTime);

	SeedCore::Vector3 moveDirection;
	SeedCore::Vector3 startPosition;

	float size;
	float speed;
	float turnSpeed;
	float aliveTime;
	float aliveTimer = 0.0f;

	int cost = 0;

	bool isStop = false;

	SeedCore::Scale* scale;
	SeedCore::Position* position;
	SeedCore::Rotation* rotation;
	SeedCore::Rigidbody* rigidbody;
	SeedCore::Velocity* velocity;

	PlayerController* playerController;
};
REGISTER_COMPONENT(BulletController);
