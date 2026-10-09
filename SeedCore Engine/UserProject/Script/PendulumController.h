#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class StopController;

class PendulumController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnInspectorGUI();
    
	SC_REFLECTION_FIELD()
		float rotateSpeed;
private:

	StopController* myStopController;
	SeedCore::Rotation* rotation;
	SeedCore::Rigidbody* rigidbody;
	SeedCore::Position* position;

	bool isFowardRotate = true;

	float rotateRate = 0.0f;

	SeedCore::Quaternion startQuaternion;
	SeedCore::Quaternion endQuaternion;
};
REGISTER_COMPONENT(PendulumController);
