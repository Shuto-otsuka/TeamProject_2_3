#pragma once
#include <FoundationEngine/SeedScript.h>

class BulletStartController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerEnter(SeedCore::Actor other);
	void OnTriggerExit(SeedCore::Actor other);

	bool IsTouch() const { return isTouch; }

private:
	bool isTouch = false;
};
REGISTER_COMPONENT(BulletStartController);
