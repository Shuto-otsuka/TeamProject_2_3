#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct EffectPlayback
	{
		Uint32 emitter_ = 0;

		Uint32 node_ = 0;

		Uint32 asset_ = 0;

		Int32 handle_ = -1;

		Bool finished_ = false;
	};

	class Effect :public SeedScript
	{
		friend class EffectSystem;
		friend class ParticleFeature;

	public:
		SC_PAYLOAD_FIELD_EX("エフェクト", Effect)
		Uint32 effectID_ = 0;

	private:
		Float age_ = 0.0f;

		Float delta_ = 0.0f;

		DynamicArray<Float> spawnAccumulators_;

		DynamicArray<Uint32> spawnCounts_;

		DynamicArray<EffectPlayback> playbacks_;
	};
	REGISTER_COMPONENT(Effect, "Graphics");
}