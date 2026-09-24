#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class Test :public SeedCore::SeedScript
{
public:
	void OnStart(); // 開始時に呼ばれる初期化処理

	void OnTick(float elapsedTime); // 更新処理

	SC_REFLECTION_FIELD()
		float speed = 5.0f;

	SeedCore::Position* position = nullptr;
};
REGISTER_COMPONENT(Test);
