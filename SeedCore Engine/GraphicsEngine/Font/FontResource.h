#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Font;
	class BindlessHeap;
	class ResourceCache;

	class FontResource :public Asset, public NonCopyable
	{
	public:
		FontResource(GraphicsLoaderSystem& loaders);
		~FontResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Font> Load(ResourceCache& cache, Uint32 assetID, Float fontSize);

		void Unload(Uint32 assetID)override;

		void Clear()override;

		Handle<Font> GetHandle(Uint32 assetID)const;

		Font* Resolve(const Handle<Font>& handle);

		Bool Contains(Uint32 assetID)const;

		void Update(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap);

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<Font>> assetHandleMap_;
	};
}
