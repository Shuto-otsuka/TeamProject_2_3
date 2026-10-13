#pragma once
#include <SeedCore/ScScript.h>

class GoalSystem :public SeedCore::SeedScript
{
public:
	SC_PAYLOAD_FIELD_EX("遷移先シーン", Scene)
	Uint32 translationSceneID_ = 0;

	SC_REFLECTION_CLAMPED_EX("範囲", 0.0f, 10000.0f)
	float range_ = 1.0f;

public:
	void OnStart();

	void OnTick(float elapsedTime);

private:
	bool isTranslationSceneReady_ = false;
};
REGISTER_COMPONENT(GoalSystem);