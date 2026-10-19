#pragma once

#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Animation/AnimationResource.h>
#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/Skeleton/SkeletonResource.h>
#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class BindlessHeap;

	struct SEEDCORE_API GraphicsResourceSystem
	{
		/// [EN] Loads texture assets from disk and uploads them to the GPU.
		/// [JP] ディスクからテクスチャアセットを読み込み、GPU へアップロードする。
		ResourcePtr<TextureResource> textureResource_;

		/// [EN] Loads 3D model assets (geometry/materials).
		/// [JP] 3Dモデルアセット（ジオメトリ/マテリアル）を読み込む。
		ResourcePtr<ModelResource> modelResource_;

		/// [EN] Loads and splits animation clip data.
		/// [JP] アニメーションクリップデータを読み込み、分割する。
		ResourcePtr<AnimationResource> animationResource_;

		/// [EN] Loads and bakes mesh collision geometry.
		/// [JP] 衝突ジオメトリを読み込み、焼き込む。
		ResourcePtr<MeshCollisionResource> meshCollisionResource_;

		/// [EN] Loads and saves standalone ".material" assets.
		/// [JP] 単体 ".material" アセットの読み書きを行う。
		ResourcePtr<MaterialResource> materialResource_;

		/// [EN] Loads and saves standalone ".skeleton" assets (rig: sockets + root bone).
		/// [JP] 単体 ".skeleton" アセット（リグ: ソケット + ルートボーン）の読み書きを行う。
		ResourcePtr<SkeletonResource> skeletonResource_;

		/// [EN] Loads font assets (FreeType/HarfBuzz/MTSDF atlas).
		/// [JP] フォントアセット（FreeType/HarfBuzz/MTSDF アトラス）を読み込む。
		ResourcePtr<FontResource> fontResource_;

		/// [EN] Loads movie assets (Media Foundation decoder + frame texture).
		/// [JP] ムービーアセット（Media Foundation デコーダ + フレームテクスチャ）を読み込む。
		ResourcePtr<MovieResource> movieResource_;

		/// [EN] Loads skybox/environment map assets.
		/// [JP] スカイボックス/環境マップアセットを読み込む。
		ResourcePtr<SkymapResource> skymapResource_;

		/// [EN] Loads Effekseer effect (.efkefc) assets.
		/// [JP] Effekseerエフェクト(.efkefc)アセットを読み込む。
		ResourcePtr<EffekseerResource> effekseerResource_;

		GraphicsResourceSystem(GraphicsLoaderSystem& loaders, BindlessHeap& heap);
	};
}