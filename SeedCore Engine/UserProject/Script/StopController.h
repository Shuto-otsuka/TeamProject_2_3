#pragma once
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class PlayerController;

class StopController :public SeedCore::SeedScript
{
	//止められるオブジェクトには共通でこのコンポネントを付ける
public:
	void OnStart();
	void OnTick(float elapsedTime);
	void OnInspectorGUI();

	//このオブジェクトが今止まっているかを取得できる関数
	bool IsStop() const { return isStop; }

	void Stop(float stopTime);
	void Move();

	SC_REFLECTION_FIELD()
		SeedCore::Vector3 stopTimerOffset;
	SC_PAYLOAD_FIELD_EX("連動アクター", Actor)
		SeedCore::DynamicArray<SeedCore::Uint32> linkActorIDs_;

private:
	bool isStop = false;

	float stopTime = 0.0f;

	SeedCore::Actor stopTimer;
	SeedCore::Actor player;
	SeedCore::Actor firstPersonCameraActor;
	SeedCore::Actor thirdPersonCameraActor;
	SeedCore::Text* stopTimerText;
	SeedCore::Position* stopTimerPosition;
	SeedCore::Position* position;
	SeedCore::AudioSource* moveAudio;
	PlayerController* playerController;
};
REGISTER_COMPONENT(StopController);
