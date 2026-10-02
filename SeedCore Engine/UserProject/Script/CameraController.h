#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class CameraController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD()
		float smoothFocusSpeed;
	SC_REFLECTION_FIELD()
		float lookPlayerHeight;
	SC_REFLECTION_FIELD()
		float distance;
	SC_REFLECTION_FIELD()
		float minDistance;
	SC_REFLECTION_FIELD()
		float maxDistance;
	SC_REFLECTION_FIELD()
		float zoomSpeed;
	SC_REFLECTION_FIELD()
		float sensitivity;
	SC_REFLECTION_FIELD()
		float pitchMin;
	SC_REFLECTION_FIELD()
		float pitchMax;

private:
	void SmoothFocus(float elapsedTime);
	void LookPlayer();
	void Rotate();
	void UpdateZoom();
	void UpdateDebug();
	void ChangeCursorMode();

	SeedCore::Vector3 smoothFocusPoint;

	bool isCursorLock = false;

	float pitch = 50.0f;
	float yaw = 180.0f;

	SeedCore::Actor cameraBrain;
	SeedCore::Actor player;
	SeedCore::Position* position;
	SeedCore::Position* playerPosition;
	SeedCore::Rotation* rotation;
};
REGISTER_COMPONENT(CameraController);
