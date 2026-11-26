#pragma once

#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Animation/AnimationResource.h>
#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/Skeleton/SkeletonResource.h>
#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Effect/EffectResource.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class BindlessHeap;
	class D3D12Context;
	class GraphicsProcessor;

	struct SEEDCORE_API GraphicsResourceSystem
	{
		ResourcePtr<TextureResource> textureResource_;

		ResourcePtr<TextureStreaming> textureStreaming_;

		ResourcePtr<ModelResource> modelResource_;

		ResourcePtr<AnimationResource> animationResource_;

		ResourcePtr<MeshCollisionResource> meshCollisionResource_;

		ResourcePtr<MaterialResource> materialResource_;

		ResourcePtr<SkeletonResource> skeletonResource_;

		ResourcePtr<FontResource> fontResource_;

		ResourcePtr<MovieResource> movieResource_;

		ResourcePtr<SkymapResource> skymapResource_;

		ResourcePtr<EffectResource> effectResource_;

		ResourcePtr<EffekseerResource> effekseerResource_;

		GraphicsResourceSystem(GraphicsLoaderSystem& loaders, D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);
	};
}