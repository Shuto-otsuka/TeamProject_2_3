#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScMath.h>
#include <SeedCore/ScComponent.h>

class DoorSystem :public SeedCore::SeedScript
{
public:
	SC_PAYLOAD_FIELD_EX("起動元回路", Actor)
	SeedCore::Uint32 circuitActorID_ = 0;

	SC_REFLECTION_CLAMPED_EX("沈む距離", 0.0f, 10000.0f)
	float sinkDistance_ = 1.0f;

	SC_REFLECTION_CLAMPED_EX("開くまでの時間", 0.0f, 10000.0f)
	float openTime_ = 1.0f;

	SC_REFLECTION_CLAMPED_EX("閉まるまでの時間", 0.0f, 10000.0f)
	float closeTime_ = 0.3f;

public:
	void OnStart();

	void OnFixedTick(float elapsedTime);

	void OnEditorTick(float elapsedTime);

private:
	SeedCore::Actor circuitActor_;

	SeedCore::Vector3 cachePosition_ = { 0.0f,0.0f,0.0f };

	SeedCore::Quaternion cacheRotation_ = SeedCore::Quaternion::Identity;

	float progress_ = 0.0f;
};
REGISTER_COMPONENT(DoorSystem);