#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class TextureResource;
	class ModelResource;
	class D3D12Context;
	class BindlessHeap;
	class GraphicsProcessor;

	class SEEDCORE_API MaterialResource :public AssetResource<BakedSurface>
	{
	public:
		MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource, ModelResource& modelResource, D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);
		~MaterialResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<BakedSurface> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		BakedSurface* Resolve(const Handle<BakedSurface>& handle);

	private:
		GraphicsLoaderSystem& loaders_;

		TextureResource& textureResource_;

		ModelResource& modelResource_;

		D3D12Context& context_;

		BindlessHeap& heap_;

		GraphicsProcessor& processor_;
	};
}
