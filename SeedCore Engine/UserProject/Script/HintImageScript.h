#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <SeedCore/ScComponent.h>

class HintImageScript :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	void Display(float fadeTime, float fadeWaitTime);

private:
	SeedCore::Image* myImage;

	float fadeTime = 0.0f;
	float fadeWaitTime = 0.0f;
	float timer = 0.0f;

	bool isDisplay = false;
};
REGISTER_COMPONENT(HintImageScript);
