#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController :public SeedCore::SeedScript
{
public:
	void OnStart(); // 開始時に呼ばれる初期化処理

	void OnTick(float elapsedTime); // 更新処理
private:
	void UpdateUsually(float elapsedTime);
	void UpdateHorizontalAcceleration(float elapsedTime);
	void UpdateInputJump(float elapsedTime);
	void Jump();

	enum class State
	{
	    USUALLY,
	};

	State state = State::USUALLY;

	SeedCore::Actor cameraBrain;
	SeedCore::Position* position = nullptr;
	SeedCore::CharacterController* myCharacterController = nullptr;
};
REGISTER_COMPONENT(PlayerController);
