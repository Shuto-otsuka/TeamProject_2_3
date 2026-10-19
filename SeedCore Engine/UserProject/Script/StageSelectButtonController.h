#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class Button;

class StageSelectButtonController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		int stageNumber = 0;
	SC_REFLECTION_FIELD()
		SeedCore::Color onCursorColor;
	SC_REFLECTION_FIELD()
		SeedCore::Color clickedColor;
	SC_REFLECTION_FIELD()
		float colorChangeSpeed;
	SC_REFLECTION_FIELD()
		float clickedSize;
	SC_REFLECTION_FIELD()
		float onCursorSize;
	SC_REFLECTION_FIELD()
		float sizeChangeSpeed;
	SC_REFLECTION_FIELD()
		float sceneChangeWaitTime;

private:
	SeedCore::Image* myImage = nullptr;
	SeedCore::Scale* scale = nullptr;
	Button* myButton = nullptr;

	SeedCore::Color defaultColor;
	SeedCore::Color targetColor;

	float defaultSize;
	float targetSize;

	bool isClicked = false;
};
REGISTER_COMPONENT(StageSelectButtonController);
