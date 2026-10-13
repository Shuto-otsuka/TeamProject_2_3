#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class LaserController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnEditorTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		SeedCore::Vector3 centerOffset;
	SC_REFLECTION_FIELD()
		SeedCore::Vector3 deltaOffset;
	SC_REFLECTION_FIELD()
		float length;
	SC_REFLECTION_FIELD()
		float tickness;
private:
	

};
REGISTER_COMPONENT(LaserController);
