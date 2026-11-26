#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerEffect.h>

namespace SeedCore
{
	struct EffekseerAsset;

	class EffekseerLoader :public AssetLoader<EffekseerEffect>
	{
	private:
		friend class EffekseerResource;

	public:
		EffekseerLoader() = default;
		~EffekseerLoader() = default;

		Handle<EffekseerEffect> Load(String filePath, LoadThread thread);

		void Clear(Handle<EffekseerEffect>& handle)noexcept;

	private:
		Bool Bake(String sourcePath, String cachePath);

		Bool Import(String sourcePath, EffekseerAsset& asset);

		Bool Read(String filePath, EffekseerAsset& asset);

		Bool Write(const EffekseerAsset& asset, String filePath);

		Bool Create(const EffekseerAsset& asset, LoadThread thread, EffekseerEffect& effect);
	};
}
