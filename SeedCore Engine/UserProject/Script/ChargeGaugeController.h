#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;


class ChargeGaugeController :public SeedCore::SeedScript
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

private:
	float GaugeToPosX(int gauge);
	float GaugeToSizeX(int gauge);
	float LerpConstantSpeed(float value, float target, float change);
	void MoveGauge(float elapsedTime);

	SeedCore::Actor chargeGaugeNow;
	SeedCore::Actor chargeGaugeKnob;
	SeedCore::Actor chargeGaugeBase;
	SeedCore::Active* chargeGaugeNowActive;
	SeedCore::Active* chargeGaugeKnobActive;
	SeedCore::Active* chargeGaugeBaseActive;
	SeedCore::Image* chargeGaugeNowImage;
	SeedCore::Position* chargeGaugeKnobPosition;

	PlayerController* playerController = nullptr;
};
REGISTER_COMPONENT(ChargeGaugeController);
