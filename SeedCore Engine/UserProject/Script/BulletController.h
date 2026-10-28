#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;
class StopController;

class BulletController :public SeedCore::SeedScript
{
public:
	//弾の状態を保存するステート
	enum class State
	{
		NORMAL,//通常飛行時
		STOP,//別の弾が当たって足場になっている状態
		STICK//ギミックにくっついている状態
	};

	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerEnter(SeedCore::Actor other);
	void OnTriggerExit(SeedCore::Actor other);

	void SetParam(const SeedCore::Vector3& startPosition,const SeedCore::Vector3& moveDirection, float chargeRate,int cost);

	void Stop();
	void Remove();
	void Move(float elapsedTime);

	const State& GetState() const { return state; }

	StopController* GetNowStick() { return nowStick; }

	float GetAliveTimer() const { return aliveTimer; }

	SC_REFLECTION_FIELD()
		float minSize;
	SC_REFLECTION_FIELD()
		float maxSize;
	SC_REFLECTION_FIELD()
		float minSpeed;
	SC_REFLECTION_FIELD()
		float maxSpeed;
	SC_REFLECTION_FIELD()
		float minAliveTime;
	SC_REFLECTION_FIELD()
		float maxAliveTime;
	SC_REFLECTION_FIELD()
		float turnSpeed;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 turnAxis;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 stopTimerOffset;

private:
	void UpdateRemove(float elapsedTime);
	void UpdateAliveTime(float elapsedTime);
	void UpdateStopTimer(float elapsedTime);

	State state = State::NORMAL;

	SeedCore::Vector3 moveDirection;
	SeedCore::Vector3 startPosition;

	float size;
	float speed;
	float aliveTime;
	float aliveTimer = 0.0f;
	float dynamicWaitTimer = 0.1f;

	int cost = 0;

	bool isGravityArea = false;
	bool isNoScaffoldArea = false;
	bool isDynamicWait = false;

	SeedCore::Scale* scale;
	SeedCore::Position* position;
	SeedCore::Rotation* rotation;
	SeedCore::Rigidbody* rigidbody;
	SeedCore::LinearVelocity* linearVelocity;
	SeedCore::AngularVelocity* angularVelocity;
	SeedCore::Actor stopTimer;
	SeedCore::Text* stopTimerText;
	SeedCore::Position* stopTimerPosition;

	SeedCore::Actor player;

	PlayerController* playerController;
	StopController* nowStick = nullptr;
};
REGISTER_COMPONENT(BulletController);
