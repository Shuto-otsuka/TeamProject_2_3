#pragma once

#include <GraphicsEngine/Texture/TextureLoader.h>
#include <GraphicsEngine/Model/ModelLoader.h>
#include <GraphicsEngine/Model/Animation/AnimationLoader.h>
#include <GraphicsEngine/Model/Collision/MeshCollisionLoader.h>
#include <GraphicsEngine/Model/Material/MaterialLoader.h>
#include <GraphicsEngine/Model/Skeleton/SkeletonLoader.h>
#include <GraphicsEngine/Font/FontLoader.h>
#include <GraphicsEngine/Movie/MovieLoader.h>
#include <GraphicsEngine/Sky/SkymapLoader.h>
#include <GraphicsEngine/Effect/EffectLoader.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>

namespace SeedCore
{
	class D3D12Context;
	class BindlessHeap;
	class GraphicsProcessor;

	struct SEEDCORE_API GraphicsLoaderSystem
	{
		ResourcePtr<TextureLoader> textureLoader_;

		ResourcePtr<ModelLoader> modelLoader_;

		ResourcePtr<AnimationLoader> animationLoader_;

		ResourcePtr<MeshCollisionLoader> meshCollisionLoader_;

		ResourcePtr<MaterialLoader> materialLoader_;

		ResourcePtr<SkeletonLoader> skeletonLoader_;

		ResourcePtr<FontLoader> fontLoader_;

		ResourcePtr<MovieLoader> movieLoader_;

		ResourcePtr<SkymapLoader> skymapLoader_;

		ResourcePtr<EffectLoader> effectLoader_;

		ResourcePtr<EffekseerLoader> effekseerLoader_;

		GraphicsLoaderSystem(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);
	};
}