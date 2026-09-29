#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Which ray-traced pass RaytracingRenderer::Dispatch records. The values
	* are listed in the order the passes run within a view's flush.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RaytracingRenderer::Dispatch がどのレイトレーシングのパスを記録するか。
	* 値は、ビューの Flush の中でパスが実行される順に並べてある。
	*/
	enum class RaytracingType : Uint32
	{
		/// [EN] Directional and punctual light shadows.
		/// [JP] ディレクショナルライトとパンクチュアルライトの影。
		Shadow,

		/// [EN] Ambient occlusion.
		/// [JP] アンビエントオクルージョン。
		AmbientOcclusion,

		/// [EN] Subsurface scattering transmittance.
		/// [JP] 表面下散乱の透過率。
		SubsurfaceScattering,

		/// [EN] Glossy reflections.
		/// [JP] 光沢反射。
		Reflection,

		/// [EN] Refraction through transmissive materials.
		/// [JP] 透過マテリアルを通る屈折。
		Refraction,

		/// [EN] One-bounce diffuse global illumination.
		/// [JP] 1 バウンスの拡散グローバルイルミネーション。
		GlobalIllumination,

		/// [EN] Volumetric clouds and the procedural sky.
		/// [JP] ボリューメトリックな雲とプロシージャル空。
		VolumetricCloudScapes,

		/// [EN] Stars and shooting stars.
		/// [JP] 星と流れ星。
		VolumetricStar,

		/// [EN] Froxel fog and volumetric light (god rays).
		/// [JP] froxel のフォグとボリューメトリックライト（ゴッドレイ）。
		VolumetricLight,
	};

	/**
	* [EN]
	* Which view's accumulation chain a ray-traced dispatch targets. Every
	* screen-space ray-traced signal (shadow, AO, ...) depends on the camera,
	* so the editor and game views each own an independent history/write
	* pair; sharing one chain would make each view's history the other view's
	* result, which the denoiser would reject every frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングのディスパッチが、どのビューの蓄積チェーンを対象に
	* するか。スクリーン空間のレイトレーシングの信号（影、AO など）はカメラに
	* 依存するため、エディタービューとゲームビューがそれぞれ独立した履歴と
	* 書き込み先の組を持つ。1 本を共有すると、各ビューの履歴がもう一方の
	* ビューの結果になり、デノイザが毎フレームそれを棄却してしまう。
	*/
	enum class RaytracingView : Uint32
	{
		/// [EN] The editor's scene view.
		/// [JP] エディターのシーンビュー。
		Editor = 0,

		/// [EN] The game view.
		/// [JP] ゲームビュー。
		Game = 1,
	};
}
