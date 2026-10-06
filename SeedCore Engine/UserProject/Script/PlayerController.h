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

	void SubCostGauge(int costGauge);

	SC_REFLECTION_FIELD()
		float acceleration;
	SC_REFLECTION_FIELD()
		float airAcceleration;
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
		float jumpInputBufferTime;
	SC_REFLECTION_FIELD()
		float coyoteTime;
	SC_REFLECTION_FIELD()
		float maxShotChargeTime;
	SC_REFLECTION_FIELD()
		float bulletOffsetY = 130.0f;
	SC_REFLECTION_FIELD()
		float minBulletOffsetZ = 10.0f;
	SC_REFLECTION_FIELD()
		float maxBulletOffsetZ = 80.0f;
	SC_REFLECTION_FIELD()
		float deadPosY = -15.0f;
	SC_REFLECTION_FIELD()
		int maxCostGauge = 100;
	SC_REFLECTION_FIELD()
		int minAddCostGauge = 10;
	SC_REFLECTION_FIELD()
		int maxAddCostGauge = 55;

private:
	void UpdateUsually(float elapsedTime);
	void UpdateHorizontalAcceleration(float elapsedTime);
	void UpdateInputJump(float elapsedTime);
	void Jump(float jumpPower);
	void UpdateCoyoteTime(float elapsedTime);
	void UpdateTurn(float elapsedTime);
	void TurnFromDirection(float elapsedTime,const SeedCore::Vector3& direction);
	void UpdateInputShot(float elapsedTime);
	void Shot();
	void UpdateFallJudge(float elapsedTime);

	bool OnGroundOrCoyote();

	const SeedCore::Vector3& RayToPlaneHitPosition(const SeedCore::Vector3& rayOrigin,
		const SeedCore::Vector3& rayDirection,
	    const SeedCore::Vector3& planeNormal,
		float planeDistance
	);

	enum class State
	{
	    USUALLY,
	};

	State state = State::USUALLY;

	bool jumpReady = false;
	bool jumpInputEnable = true;
	bool jumpInputBuffer = false;
	bool beforeIsGround = true;
	bool isCoyote = false;
	bool shotReady = false;

	float jumpInputBufferTimer = 0.0f;
	float inputBufferJumpPower = 0.0f;
	float jumpInputEnableTimer = 0.0f;
	float jumpInputTimer = 0.0f;
	float coyoteTimer = 0.0f;
	float shotInputTimer = 0.0f;

	int costGauge = 0;

	SeedCore::Vector3 lookDirection = { 0.0f,0.0f,1.0f };
	SeedCore::Vector3 shotDirection = { 0.0f,0.0f,1.0f };

	SeedCore::Actor cameraBrain;
	SeedCore::Position* position = nullptr;
	SeedCore::Rotation* rotation = nullptr;
	SeedCore::Scale* scale = nullptr;
	SeedCore::CharacterController* myCharacterController = nullptr;
};
REGISTER_COMPONENT(PlayerController);
