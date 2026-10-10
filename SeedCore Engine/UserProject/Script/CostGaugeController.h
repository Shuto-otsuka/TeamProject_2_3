#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

class CostGaugeController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float posXMin;
	SC_REFLECTION_FIELD()
		float posXMax;
	SC_REFLECTION_FIELD()
		float sizeXMin;
	SC_REFLECTION_FIELD()
		float sizeXMax;
	SC_REFLECTION_FIELD()
		float gaugeMoveSpeed;

private:
	float GaugeToPosX(int gauge);
	float GaugeToSizeX(int gauge);
	float LerpConstantSpeed(float value, float target,float change);
	void MoveGauge(float elapsedTime);

	SeedCore::Actor costGaugeUsed;
	SeedCore::Actor costGaugeAdd;
	SeedCore::Image* costGaugeUsedImage;
	SeedCore::Image* costGaugeAddImage;
	SeedCore::Position* costGaugeAddPosition;

	PlayerController* playerController = nullptr;

	float usedSizeXTarget;
	float addPosXTarget;
	float addSizeXTarget;
};
REGISTER_COMPONENT(CostGaugeController);
