#pragma once

namespace SeedCore
{
	class SEEDCORE_API EffekseerEffect :public NonCopyable
	{
		friend class EffekseerLoader;

	public:
		EffekseerEffect() = default;
		~EffekseerEffect()noexcept = default;

		EffekseerEffect(EffekseerEffect&& other)noexcept;
		EffekseerEffect& operator=(EffekseerEffect&& other)noexcept;

	public:
		NODISCARD Effekseer::EffectRef& Effect();

	private:
		Effekseer::EffectRef effect_;
	};
}
