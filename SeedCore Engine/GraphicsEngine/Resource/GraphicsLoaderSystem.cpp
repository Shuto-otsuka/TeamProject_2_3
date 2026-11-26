#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	GraphicsLoaderSystem::GraphicsLoaderSystem(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor)
	{
		textureLoader_ = MakePtr<TextureLoader>(context, heap);
		modelLoader_ = MakePtr<ModelLoader>(context, heap, processor);
		animationLoader_ = MakePtr<AnimationLoader>();
		meshCollisionLoader_ = MakePtr<MeshCollisionLoader>();
		materialLoader_ = MakePtr<MaterialLoader>();
		skeletonLoader_ = MakePtr<SkeletonLoader>();
		fontLoader_ = MakePtr<FontLoader>();
		movieLoader_ = MakePtr<MovieLoader>();
		skymapLoader_ = MakePtr<SkymapLoader>(context, heap);
		effectLoader_ = MakePtr<EffectLoader>();
		effekseerLoader_ = MakePtr<EffekseerLoader>();
	}
}