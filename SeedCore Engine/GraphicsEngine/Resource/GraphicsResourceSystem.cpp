#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates every resource manager, handing each the loaders and the
	* other managers or GPU objects it depends on. The model manager is
	* created before the managers that generate their files from a
	* model's Crister.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのリソースマネージャを生成し、それぞれにローダーと、依存する
	* 他のマネージャや GPU のオブジェクトを渡す。モデルの Crister から
	* ファイルを作るマネージャより先に、モデルのマネージャを生成する。
	*/
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
		effekseerResource_ = MakePtr<EffekseerResource>(loaders);
	}
}