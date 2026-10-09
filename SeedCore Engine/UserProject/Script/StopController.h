#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

class StopController :public SeedCore::SeedScript
{
	//止められるオブジェクトには共通でこのコンポネントを付ける
public:
	void OnStart();
	void OnTick(float elapsedTime);

	//このオブジェクトが今止まっているかを取得できる関数
	bool IsStop() const { return isStop; }

	void Stop(float stopTime);

private:
	bool isStop = false;

	float stopTime = 0.0f;
};
REGISTER_COMPONENT(StopController);
