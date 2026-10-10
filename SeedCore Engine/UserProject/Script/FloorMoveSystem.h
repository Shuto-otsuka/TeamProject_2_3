#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScComponent.h>

class StopController;

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

	SC_REFLECTION_FIELD_EX("床からのProbeの相対位置")
	SeedCore::DynamicArray<SeedCore::Vector3> localPositions_;

	SC_REFLECTION_CLAMPED_EX("速度", 0.0f, 10000.0f)
	float scalarSpeed_ = 1.0f;

	SC_REFLECTION_FIELD_EX("端での待ち時間")
	float waitTime_ = 0.5f;

	SC_REFLECTION_FIELD_EX("デバッグアローを描画するか")
	bool isDebugArrowDraw_ = true;

public:
	void OnStart();

	void OnFixedTick(float elapsedTime);

	void OnEditorTick(float elapsedTime);

private:
	StopController* stopController_ = nullptr;

	SeedCore::Vector3 currentTarget_ = { 0.0f,0.0f,0.0f };

	SeedCore::Vector3 cachePosition_ = { 0.0f,0.0f,0.0f };

	SeedCore::Quaternion cacheRotation_ = SeedCore::Quaternion::Identity;

	int startIndex_ = 0;

	int pingPongDirection_ = 1;

	float progress_ = 0.0f;

	float waitTimer_ = 0.0f;

	bool isMoveLock_ = false;
};
REGISTER_COMPONENT(FloorMoveSystem);