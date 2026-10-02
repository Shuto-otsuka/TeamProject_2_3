#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	class Effect :public SeedScript
	{
		friend class EffectSystem;
		friend class EffectRenderer;

	public:
		SC_REFLECTION_FIELD_EX("スポーンレート")
		Float spawnRate_ = 1.0f;

	private:
		Float age_ = 0.0f;

		Float spawnAccumulator_ = 0.0f;

		Uint32 spawnCount_ = 1000;
	};
	REGISTER_COMPONENT(Effect, "Graphics");
}