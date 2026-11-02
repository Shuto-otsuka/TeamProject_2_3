#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs the loader system, creating each format-specific loader
	* and handing the GPU-backed ones the device context, bindless heap
	* and BC7 compute shader they create their objects with.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーシステムを構築し、各フォーマット固有のローダーを生成する。
	* GPU を使うローダーには、オブジェクトを作るためのデバイスの
	* コンテキスト、バインドレスヒープ、BC7 のコンピュートシェーダーを渡す。
	*/
	GraphicsLoaderSystem::GraphicsLoaderSystem(D3D12Context& context, BindlessHeap& heap, BC7CompressShader& bc7Shader)
	{
		textureLoader_ = MakePtr<TextureLoader>(context, heap);
		modelLoader_ = MakePtr<ModelLoader>(context, heap, bc7Shader);
		animationLoader_ = MakePtr<AnimationLoader>();
		meshCollisionLoader_ = MakePtr<MeshCollisionLoader>();
		materialLoader_ = MakePtr<MaterialLoader>();
		skeletonLoader_ = MakePtr<SkeletonLoader>();
		fontLoader_ = MakePtr<FontLoader>();
		movieLoader_ = MakePtr<MovieLoader>();
		skymapLoader_ = MakePtr<SkymapLoader>(context, heap);
		effekseerLoader_ = MakePtr<EffekseerLoader>();
	}
}