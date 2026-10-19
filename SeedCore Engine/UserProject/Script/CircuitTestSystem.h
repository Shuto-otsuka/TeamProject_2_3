#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScMath.h>
#include <SeedCore/ScComponent.h>

class CircuitTestSystem :public SeedCore::SeedScript
{
public:
	SC_PAYLOAD_FIELD_EX("確認する回路", Actor)
	SeedCore::Uint32 circuitActorID_ = 0;

	SC_REFLECTION_CLAMPED_EX("浮く高さ", 0.0f, 10000.0f)
	float floatHeight_ = 1.5f;

	SC_REFLECTION_CLAMPED_EX("浮き沈みにかかる時間", 0.0f, 10000.0f)
	float moveTime_ = 0.6f;

	SC_REFLECTION_FIELD_EX("通電中の回転速度(度/秒)")
	float rotateSpeed_ = 180.0f;

	SC_REFLECTION_FIELD_EX("変化をログに出すか")
	bool isLogOutput_ = true;

public:
	void OnStart();

	void OnTick(float elapsedTime);

	void OnInspectorGUI();

private:
	SeedCore::Actor circuitActor_;

	SeedCore::Vector3 cachePosition_ = { 0.0f,0.0f,0.0f };

	SeedCore::Quaternion cacheRotation_ = SeedCore::Quaternion::Identity;

	float progress_ = 0.0f;

	float angle_ = 0.0f;

	bool wasPowered_ = false;
};
REGISTER_COMPONENT(CircuitTestSystem);
