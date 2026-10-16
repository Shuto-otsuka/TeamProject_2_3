#pragma once
#include <SeedCore/ScScript.h>

class CircuitSystem :public SeedCore::SeedScript
{
public:
	enum class Logic
	{
		And,
		Or,
		Count,
	};

public:
	SC_PAYLOAD_FIELD_EX("入力アクター", Actor)
	SeedCore::DynamicArray<SeedCore::Uint32> inputActorIDs_;

	SC_REFLECTION_FIELD_EX("論理")
	Logic logic_ = Logic::And;

	SC_REFLECTION_CLAMPED_EX("起動最低数", 0, 100000)
	int guaranteeCount_ = 1;

	SC_REFLECTION_FIELD_EX("結果を反転")
	bool flipResult_ = false;

	SC_REFLECTION_FIELD_EX("結果を保持")
	bool keepResult_ = false;

public:
	void OnStart();

	void OnLateTick(float elapsedTime);

	void OnDestroy();

public:
	bool IsPowered()const;

private:
	SeedCore::DynamicArray<SeedCore::Actor> cacheActors_;

	bool powered_ = false;

	bool latched_ = false;
};
REGISTER_COMPONENT(CircuitSystem);