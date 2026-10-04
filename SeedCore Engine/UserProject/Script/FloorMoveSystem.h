#pragma once
#include <SeedCore/ScScript.h>

class FloorMoveSystem :public SeedCore::SeedScript
{
public:
	enum class MoveType
	{
		PingPong,
		Repeat,
	};

public:
	SC_REFLECTION_FIELD_EX("動き方")
	MoveType moveType_ = MoveType::PingPong;

	SC_REFLECTION_FIELD_EX("端での待ち時間")
	float waitTime_ = 0.5f;

public:
	void OnStart();

	void OnFixedTick(float elapsedTime);
};
REGISTER_COMPONENT(FloorMoveSystem);