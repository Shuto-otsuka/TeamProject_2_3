#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class BallController;

class BallReseterController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	void Reset();

	SC_PAYLOAD_FIELD_EX("対象ボール", Actor)
		SeedCore::Uint32 ballActorID;

private:
	SeedCore::Actor targetBall;

	BallController* targetBallController;
};
REGISTER_COMPONENT(BallReseterController);
