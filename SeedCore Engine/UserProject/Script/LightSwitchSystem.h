#pragma once
#include <SeedCore/ScScript.h>

class LightSwitchSystem :public SeedCore::SeedScript
{
public:
	SC_REFLECTION_FIELD_EX("回転速度")
	float rotateSpeed_ = 1.0f;

	SC_REFLECTION_FIELD_EX("反応距離")
	float detectDistance_ = 1.0f;

	SC_REFLECTION_FIELD_EX("点灯を保持")
	bool keepResult_ = true;

public:
	void OnStart();

	void OnTick(float elapsedTime);

public:
	bool IsOn()const;

private:
	bool isOn_ = false;

	SeedCore::Uint32 offMaterialID_ = 0;

	SeedCore::Uint32 onMaterialID_ = 0;
};
REGISTER_COMPONENT(LightSwitchSystem);
