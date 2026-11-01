#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	GraphicsResourceSystem::GraphicsResourceSystem(GraphicsLoaderSystem& loaders, D3D12Context& context, BindlessHeap& heap, BC7CompressShader& bc7Shader)
	{
		textureResource_ = MakePtr<TextureResource>(loaders, context, heap);
		modelResource_ = MakePtr<ModelResource>(loaders, context, heap, bc7Shader);
		animationResource_ = MakePtr<AnimationResource>(loaders);
		meshCollisionResource_ = MakePtr<MeshCollisionResource>(loaders);
		materialResource_ = MakePtr<MaterialResource>(loaders, *textureResource_);
		skeletonResource_ = MakePtr<SkeletonResource>(loaders);
		fontResource_ = MakePtr<FontResource>(loaders);
		movieResource_ = MakePtr<MovieResource>(loaders);
		skymapResource_ = MakePtr<SkymapResource>(loaders, context, heap);
		effekseerResource_ = MakePtr<EffekseerResource>(loaders);
	}
}