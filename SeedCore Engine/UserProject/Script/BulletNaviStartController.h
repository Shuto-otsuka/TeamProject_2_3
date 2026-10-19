#pragma once
#include <FoundationEngine/SeedScript.h>

class BulletNaviStartController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerStay(SeedCore::Entity entity);
	void OnTriggerExit(SeedCore::Entity entity);

	bool IsTouch() const { return isTouch; }

private:
	bool isTouch = false;
};
REGISTER_COMPONENT(BulletNaviStartController);
