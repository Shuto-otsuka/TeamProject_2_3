#pragma once
#include <GraphicsEngine/Effect/EffectAsset.h>
#include <FoundationEngine/Resource/Asset/AssetLoader.h>

namespace SeedCore
{
	class SEEDCORE_API EffectLoader :public AssetLoader<EffectAsset>
	{
	private:
		friend class EffectResource;

	public:
		EffectLoader() = default;
		~EffectLoader() = default;

		Handle<EffectAsset> Load(String filePath, LoadThread thread);

		void Clear(Handle<EffectAsset>& handle)noexcept;

	private:
		Bool Bake(const EffectAsset& effect, String filePath);
	};
}