#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class AreaController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float fadeSpeed = 5.0f;
	SC_PAYLOAD_FIELD(Material)
		SeedCore::Uint32 lerpMaterial;
private:
	float radian = 0.0f;

	SeedCore::Material* myMaterial;
};
REGISTER_COMPONENT(AreaController);
