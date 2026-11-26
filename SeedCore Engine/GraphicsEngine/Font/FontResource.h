#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Font;
	class ResourceCache;

	class FontResource :public AssetResource<Font>
	{
	private:
		friend class FontSystem;

	public:
		FontResource(GraphicsLoaderSystem& loaders);
		~FontResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Font> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		void Clear()override;

		Font* Resolve(const Handle<Font>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
