#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScComponent.h>

class GoalSystem :public SeedCore::SeedScript
{
public:
	SC_PAYLOAD_FIELD_EX("遷移先シーン", Scene)
	Uint32 nextSceneID_ = 0;

	SC_PAYLOAD_FIELD_EX("タイトルシーン", Scene)
	Uint32 titleSceneID_ = 0;

	SC_PAYLOAD_FIELD_EX("リトライ先シーン", Scene)
	Uint32 retrySceneID_ = 0;

	SC_REFLECTION_CLAMPED_EX("範囲", 0.0f, 10000.0f)
	float range_ = 1.0f;

public:
	void OnStart();

	void OnTick(float elapsedTime);

	void OnEditorTick(float elapsedTime);

private:
	SC_CONSTEX float directionTime_ = 0.4f;

	bool isGoal_ = false;

	bool isDecided_ = false;

	float resultTimer_ = 0.0f;

	SeedCore::Actor resultActor_;
	SeedCore::Actor toNextSceneActor_;
	SeedCore::Actor toTitleSceneActor_;
	SeedCore::Actor toRetrySceneActor_;
};
REGISTER_COMPONENT(GoalSystem);