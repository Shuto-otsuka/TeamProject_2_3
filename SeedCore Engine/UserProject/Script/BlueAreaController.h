#pragma once
#include <SeedCore/ScScript.h>

class BlueAreaController :SeedCore::SeedScript
{
public:
	SC_REFLECTION_CLAMPED_EX("フェード時間", 0.0f, 10000.0f)
	float fadeTime_ = 1.0f;

	SC_REFLECTION_CLAMPED_EX("切り替わり時間", 0.0f, 10000.0f)
	float switchTime_ = 3.0f;

public:
	void OnStart();

	void OnTick(float elapsedTime);

private:
	SeedCore::Uint32 ownerMaterialID = 0;

	SeedCore::Uint32 transparentMaterialID = 0;

	float fadeTimer_ = 0.0f;

	int isTransparent = 0;
};
REGISTER_COMPONENT(BlueAreaController);