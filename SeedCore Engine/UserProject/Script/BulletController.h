#pragma once
#include <FoundationEngine/Prelude.h>
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
	void OnTriggerEnter(SeedCore::Entity entity);

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
		float removeDistance;
	SC_REFLECTION_FIELD()
		float turnSpeed;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 turnAxis;

private:
	void UpdateRemove(float elapsedTime);
	void UpdateAliveTime(float elapsedTime);

	State state = State::NORMAL;

	SeedCore::Vector3 moveDirection;
	SeedCore::Vector3 startPosition;

	float size;
	float speed;
	float aliveTime;
	float aliveTimer = 0.0f;

	int cost = 0;

	SeedCore::Scale* scale;
	SeedCore::Position* position;
	SeedCore::Rotation* rotation;
	SeedCore::Rigidbody* rigidbody;
	SeedCore::Velocity* velocity;

	SeedCore::Actor player;

	PlayerController* playerController;
	StopController* nowStick = nullptr;
};
REGISTER_COMPONENT(BulletController);
