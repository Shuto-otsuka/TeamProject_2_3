#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController :public SeedCore::SeedScript
{
public:
	void OnStart(); // 開始時に呼ばれる初期化処理

	void OnTick(float elapsedTime); // 更新処理
	void OnInspectorGUI();

	SC_REFLECTION_FIELD()
	    float turnSpeed;
	SC_REFLECTION_FIELD()
		float minJumpPower;
	SC_REFLECTION_FIELD()
		float maxJumpPower;
	SC_REFLECTION_FIELD()
		float maxJumpInputTime;
	SC_REFLECTION_FIELD()
		float jumpEnableTime;
	SC_REFLECTION_FIELD()
		float coyoteTime;

private:
	void UpdateUsually(float elapsedTime);
	void UpdateHorizontalAcceleration(float elapsedTime);
	void UpdateInputJump(float elapsedTime);
	void Jump(float jumpPower);
	void UpdateCoyoteTime(float elapsedTime);
	void Turn(float elapsedTime);

	bool OnGroundOrCoyote();

	enum class State
	{
	    USUALLY,
	};

	State state = State::USUALLY;

	bool jumpReady = false;
	bool beforeIsGround = true;
	bool isCoyote = false;
	bool jumpInputEnable = true;

	float jumpInputEnableTimer = 0.0f;
	float jumpInputTimer = 0.0f;
	float coyoteTimer = 0.0f;

	SeedCore::Vector3 lookDirection = { 0.0f,0.0f,1.0f };

	SeedCore::Actor cameraBrain;
	SeedCore::Position* position = nullptr;
	SeedCore::Rotation* rotation = nullptr;
	SeedCore::CharacterController* myCharacterController = nullptr;
};
REGISTER_COMPONENT(PlayerController);
