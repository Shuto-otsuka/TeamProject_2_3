#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	GraphicsModule::GraphicsModule(BindlessHeap& heap) :resources_(loaders_, heap)
	{
		/// No Code
	}

	void GraphicsModule::Install(ResourceCache& cache)
	{
		cache.Register(AssetType::Texture, *resources_.textureResource_);
		cache.Register(AssetType::Model, *resources_.modelResource_);
		cache.Register(AssetType::Animation, *resources_.animationResource_);
		cache.Register(AssetType::MeshCollision, *resources_.meshCollisionResource_);
		cache.Register(AssetType::Material, *resources_.materialResource_);
		cache.Register(AssetType::Skeleton, *resources_.skeletonResource_);
		cache.Register(AssetType::Font, *resources_.fontResource_);
		cache.Register(AssetType::Movie, *resources_.movieResource_);
		cache.Register(AssetType::Skymap, *resources_.skymapResource_);
		cache.Register(AssetType::Effect, *resources_.effekseerResource_);
	}

	GraphicsLoaderSystem& GraphicsModule::Loaders()
	{
		return loaders_;
	}

	GraphicsResourceSystem& GraphicsModule::Resources()
	{
		return resources_;
	}
}