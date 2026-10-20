#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class Button :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnInspectorGUI();

	bool IsClick() const { return isClick; }
	bool IsOnCursor() const { return isOnCursor; }

private:
	SeedCore::Position* position;
	SeedCore::Image* myImage;
	SeedCore::Scale* scale;

	SeedCore::Vector2 minOffset;
	SeedCore::Vector2 maxOffset;

	bool isClick = false;
	bool isOnCursor = false;
};
REGISTER_COMPONENT(Button);
