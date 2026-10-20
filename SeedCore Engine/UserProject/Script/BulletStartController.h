#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

class BulletStartController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnTriggerEnter(SeedCore::Entity entity);
	void OnTriggerExit(SeedCore::Entity entity);

	bool IsTouch() const { return isTouch; }

private:
	bool isTouch = false;
};
REGISTER_COMPONENT(BulletStartController);
