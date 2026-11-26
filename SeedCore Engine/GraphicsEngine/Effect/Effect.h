#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct EffectPlayback
	{
		Uint32 emitter_ = 0;

		Uint32 cycle_ = 0;

		Uint32 node_ = 0;

		Uint32 asset_ = 0;

		Int32 current_ = -1;

		Int32 previous_ = -1;

		Bool finished_ = false;
	};

	enum class EffectStopFlag :Uint8
	{
		Finish,
		Immediate,
	};

	class Effect :public SeedScript
	{
		friend class EffectSystem;
		friend class ParticleFeature;

	public:
		SC_PAYLOAD_FIELD_EX("エフェクト", Effect)
		Uint32 effectID_ = 0;

		SC_REFLECTION_FIELD_EX("自動再生")
		Bool autoPlay_ = true;

	public:
		SEEDCORE_API void OnAwake();

		SEEDCORE_API void OnDestroy();

	public:
		SEEDCORE_API void Play();

		SEEDCORE_API void Stop(EffectStopFlag flag = EffectStopFlag::Finish);

	public:
		SEEDCORE_API Bool Playing()const;

	private:
		Bool playing_ = false;

		Float age_ = 0.0f;

		Float delta_ = 0.0f;

		DynamicArray<Float> spawnAccumulators_;

		DynamicArray<Uint32> spawnCounts_;

		DynamicArray<EffectPlayback> playbacks_;
	};
	REGISTER_COMPONENT(Effect, "Graphics");
}