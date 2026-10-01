#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController :public SeedCore::SeedScript
{
public:
	void OnStart(); // 開始時に呼ばれる初期化処理

	void OnTick(float elapsedTime); // 更新処理

	SC_REFLECTION_FIELD()
		float acceleration = 10.0f;
private:
	void UpdateUsually(float elapsedTime);

	enum class State
	{
	    USUALLY,
	};

	State state = State::USUALLY;

	SeedCore::Position* position = nullptr;
	SeedCore::Rigidbody* myRigid = nullptr;
	SeedCore::Velocity* myVelocity = nullptr;
};
REGISTER_COMPONENT(PlayerController);
