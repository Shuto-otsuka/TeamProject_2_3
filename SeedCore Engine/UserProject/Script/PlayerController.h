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

	int GetCostGaugeUsed() const { return costGauge; }
	int GetCostGaugeAdd() const { return costGaugeAdd; }
	int GetCostGaugeMax() const { return maxCostGauge; }
	float GetChargeRate() const { return shotChargeRate; }

	SC_REFLECTION_FIELD()
		float acceleration;
	SC_REFLECTION_FIELD()
		float airAcceleration;
	SC_REFLECTION_FIELD()
	    float turnSpeed;
	SC_REFLECTION_FIELD()
		float jumpEnableTime;
	SC_REFLECTION_FIELD()
		float jumpInputBufferTime;
	SC_REFLECTION_FIELD()
		float maxJumpInputTime = 0.2f;
	SC_REFLECTION_FIELD()
		float jumpGravityScaler = 2.3f;
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
	SC_REFLECTION_FIELD()
	    float resetInputTime;

private:
	void UpdateUsually(float elapsedTime);
	void UpdateHorizontalAcceleration(float elapsedTime);
	void UpdateInputJump(float elapsedTime);
	void Jump();
	void UpdateCoyoteTime(float elapsedTime);
	void UpdateTurn(float elapsedTime);
	void TurnFromDirection(float elapsedTime,const SeedCore::Vector3& direction);
	void UpdateInputShot(float elapsedTime);
	void Shot();
	void UpdateFallJudge(float elapsedTime);
	void UpdateAllBulletRemove(float elapsedTime);
	void UpdateInputReset(float elapsedTime);

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

	bool jumpInputEnable = true;
	bool jumpInputBuffer = false;
	bool isJumpKeyReleaseWait = false;
	bool beforeIsGround = true;
	bool isCoyote = false;
	bool shotReady = false;

	float defaultGravity = 0.0f;
	float jumpInputBufferTimer = 0.0f;
	float jumpInputEnableTimer = 0.0f;
	float jumpInputTimer = 0.0f;
	float coyoteTimer = 0.0f;
	float shotInputTimer = 0.0f;
	float resetInputTimer = 0.0f;
	float shotChargeRate = 0.0f;

	int costGauge = 0;
	int costGaugeAdd = 0;

	SeedCore::Vector3 lookDirection = { 0.0f,0.0f,1.0f };
	SeedCore::Vector3 shotDirection = { 0.0f,0.0f,1.0f };

	SeedCore::Actor cameraBrain;
	SeedCore::Position* position = nullptr;
	SeedCore::Rotation* rotation = nullptr;
	SeedCore::Scale* scale = nullptr;
	SeedCore::CharacterController* myCharacterController = nullptr;
	SeedCore::Velocity* velocity = nullptr;
};
REGISTER_COMPONENT(PlayerController);
