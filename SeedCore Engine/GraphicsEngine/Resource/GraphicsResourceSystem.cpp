#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	GraphicsResourceSystem::GraphicsResourceSystem(GraphicsLoaderSystem& loaders, D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor)
	{
		textureResource_ = MakePtr<TextureResource>(loaders);
		textureStreaming_ = MakePtr<TextureStreaming>(*textureResource_, context, heap);
		modelResource_ = MakePtr<ModelResource>(loaders);
		animationResource_ = MakePtr<AnimationResource>(loaders);
		meshCollisionResource_ = MakePtr<MeshCollisionResource>(loaders, *modelResource_);
		materialResource_ = MakePtr<MaterialResource>(loaders, *textureResource_, *modelResource_, context, heap, processor);
		skeletonResource_ = MakePtr<SkeletonResource>(loaders, *modelResource_);
		fontResource_ = MakePtr<FontResource>(loaders);
		movieResource_ = MakePtr<MovieResource>(loaders);
		skymapResource_ = MakePtr<SkymapResource>(loaders);
		effectResource_ = MakePtr<EffectResource>(loaders);
		effekseerResource_ = MakePtr<EffekseerResource>(loaders);
	}
}