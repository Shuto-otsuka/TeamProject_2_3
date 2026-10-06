#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/Raytracing/RaytracingContext.h>
#include <GraphicsEngine/ScreenSpace/ScreenSpaceContext.h>
#include <GraphicsEngine/Rasterization/RasterizationContext.h>
#include <GraphicsEngine/Quality/GraphicsQuality.h>

namespace SeedCore
{
	/**
	* [EN]
	* Rendering settings saved with the scene, the editor-side counterpart
	* of the scene file's SceneVisual: loaded from it when a scene opens and
	* written back into it when the scene is saved. Meant to be removed once
	* these settings become components on actors in the world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンと一緒に保存する描画の設定。シーンファイルの SceneVisual に対応する
	* エディタ側のもので、シーンを開くときにそこから読み込み、保存するときに
	* 書き戻す。これらの設定がワールド内のアクターのコンポーネントになったら
	* 削除する予定。
	*/
	struct SceneVisualContext
	{
		/// [EN] Ray-traced effects: shadows, reflections, global illumination and the rest.
		/// [JP] レイトレーシングによる効果。影・反射・グローバルイルミネーションなど。
		RaytracingContext raytracing_;

		/// [EN] Screen-space effects.
		/// [JP] スクリーンスペースの効果。
		ScreenSpaceContext screenSpace_;

		/// [EN] Rasterization settings.
		/// [JP] ラスタライズの設定。
		RasterizationContext rasterization_;

		/// [EN] Quality preset that sets the three groups above together; effects can be switched on or off by hand only while it is Custom.
		/// [JP] 上の3つをまとめて設定する品質プリセット。効果を手でオン・オフできるのは Custom のときだけ。
		GraphicsQualityPreset qualityPreset_ = GraphicsQualityPreset::Custom;
	};
}
