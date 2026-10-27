#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class BombExplosionController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	void SetParam(const SeedCore::Vector3& pos, float targetSize);

	SC_REFLECTION_FIELD()
		float scaleUpTime = 0.2f;
	SC_REFLECTION_FIELD()
		float scaleDownTime = 0.6f;
	SC_REFLECTION_FIELD()
		float waitTime = 0.5f;

private:
	enum class State
	{
		SCALE_UP,
		WAIT,
		SCALE_DOWN,
	};

	State state;

	float currentSize = 0.0f;
	float targetSize = 0.0f;
	float sizeRate = 0.0f;

	bool isScaleUp = true;

	SeedCore::Position* position;
	SeedCore::Scale* scale;
};
REGISTER_COMPONENT(BombExplosionController);
