#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScComponent.h>

class HintSystem :public SeedCore::SeedScript
{
public:
	SC_REFLECTION_FIELD_EX("回転速度")
	float rotateSpeed_ = 1.0f;

	SC_REFLECTION_FIELD_EX("表示維持時間")
	float maintainTime_ = 5.0f;

	SC_REFLECTION_FIELD_EX("フェード時間")
	float fadeTime_ = 3.0f;

	SC_REFLECTION_FIELD_EX("ヒント文")
	SeedCore::String hintText_;

public:
	void OnStart();

	void OnTick(float elapsedTime);

public:
	void Startup();

private:
	SeedCore::Actor textActor_;

	SeedCore::Actor standActor_;

	SeedCore::Actor glassActor_;

	SeedCore::Uint32 offMaterialID_ = 0;

	SeedCore::Uint32 onMaterialID_ = 0;

	float timer_ = 0.0f;

	bool startup_ = false;
};
REGISTER_COMPONENT(HintSystem);
