#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

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

private:
	bool isStop = false;

	float stopTime = 0.0f;

	SeedCore::Actor stopTimer;
	SeedCore::Text* stopTimerText;
	SeedCore::Position* stopTimerPosition;
	SeedCore::Position* position;
};
REGISTER_COMPONENT(StopController);
