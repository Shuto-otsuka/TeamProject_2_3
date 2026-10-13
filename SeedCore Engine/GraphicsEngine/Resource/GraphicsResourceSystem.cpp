#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	GraphicsResourceSystem::GraphicsResourceSystem(GraphicsLoaderSystem& loaders, BindlessHeap& heap)
	{
		textureResource_ = MakePtr<TextureResource>(loaders, heap);
		modelResource_ = MakePtr<ModelResource>(loaders, heap);
		animationResource_ = MakePtr<AnimationResource>(loaders);
		meshCollisionResource_ = MakePtr<MeshCollisionResource>(loaders);
		materialResource_ = MakePtr<MaterialResource>(loaders);
		skeletonResource_ = MakePtr<SkeletonResource>(loaders);
		fontResource_ = MakePtr<FontResource>(loaders);
		movieResource_ = MakePtr<MovieResource>(loaders);
		skymapResource_ = MakePtr<SkymapResource>(loaders, heap);
		effekseerResource_ = MakePtr<EffekseerResource>(loaders);
	}
}