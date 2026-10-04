#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class BulletController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void SetParam(SeedCore::Vector3 startPosition,SeedCore::Vector3 moveDirection, float chargeTime);

	SC_REFLECTION_FIELD()
		float maxChargeTime;
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

private:
	void Move(float elapsedTime);
	void UpdateAliveTime(float elapsedTime);

	SeedCore::Vector3 moveDirection;
	float size;
	float speed;
	float aliveTime;

	SeedCore::Scale* scale;
	SeedCore::Position* position;
};
REGISTER_COMPONENT(BulletController);
