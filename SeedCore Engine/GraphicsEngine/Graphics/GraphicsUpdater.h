#pragma once
#include <GraphicsEngine/System/AnimationSystem.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/System/ColliderSystem.h>
#include <GraphicsEngine/System/ConstraintSystem.h>
#include <GraphicsEngine/System/EffectSystem.h>
#include <GraphicsEngine/System/FontSystem.h>
#include <GraphicsEngine/System/MaterialSystem.h>
#include <GraphicsEngine/System/MovieSystem.h>
#include <GraphicsEngine/System/ShapeSystem.h>

namespace SeedCore
{
	struct GraphicsSettings;

	class GraphicsModule;
	class RenderDevice;
	class World;

	/**
	* [EN]
	* Advances the per-frame state that rendering reads, once per frame
	* before the views are added: fonts, movies, effects, the celestial
	* clock, colliders and shapes, then constraints, animation, constraints
	* again and materials.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 描画が読むフレームごとの状態を、ビューを足す前にフレームに 1 回
	* 進める。フォント、ムービー、エフェクト、天体の時計、コライダー、形、
	* 続いてコンストレイント、アニメーション、もう一度コンストレイント、
	* マテリアルの順。
	*/
	class GraphicsUpdater
	{
	public:
		/**
		* [EN]
		* Keeps the device fonts and movies are uploaded with, and the asset
		* module the systems read their resources from.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フォントとムービーのアップロードに使うデバイスと、System が
		* リソースを読むアセットモジュールを受け取る。
		*/
		GraphicsUpdater(RenderDevice& device, GraphicsModule& module);

		/**
		* [EN]
		* Destroys the updater.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 更新器を破棄する。
		*/
		~GraphicsUpdater();

	public:
		/**
		* [EN]
		* Records whether the shape debug display covers every actor rather
		* than only the selected ones.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 形のデバッグ表示を、選択中だけでなく全アクターに出すかを記録する。
		*/
		void Configure(const GraphicsSettings& settings);

	public:
		/**
		* [EN]
		* Advances the per-frame state of world by deltaTime. Called once per
		* frame, before the views are added, since every view of a frame
		* reads the same state. aspectRatio is the render aspect ratio the
		* camera frustums of the shape display use.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world のフレームごとの状態を deltaTime だけ進める。フレームの全
		* ビューが同じ状態を読むので、ビューを足す前にフレームに 1 回呼ぶ。
		* aspectRatio は、形の表示のカメラの視錐台が使う描画の縦横比。
		*/
		void Update(Float deltaTime, World& world, Float aspectRatio);

		/**
		* [EN]
		* Returns this frame's sun, moon and night factor, as computed by the
		* last Update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最後の Update が求めた、このフレームの太陽・月・夜の度合いを返す。
		*/
		NODISCARD const CelestialResult& Celestial()const;

	private:
		/// [EN] One instance per system type; std::get by type finds a system at compile time.
		/// [JP] System の型ごとに 1 つ。型で std::get すればコンパイル時に System が決まる。
		using Systems = std::tuple
			<
			FontSystem,
			MovieSystem,
			EffectSystem,
			CelestialSystem,
			ColliderSystem,
			ShapeSystem,
			ConstraintSystem,
			AnimationSystem,
			MaterialSystem
			>;

	private:
		/// [EN] Device fonts and movies are uploaded with.
		/// [JP] フォントとムービーをアップロードするデバイス。
		RenderDevice& device_;

		/// [EN] Asset module the systems read their resources from.
		/// [JP] System がリソースを読むアセットモジュール。
		GraphicsModule& module_;

		/// [EN] Whether the shape debug display covers every actor; set by the editor every frame, off in the game runtime.
		/// [JP] 形のデバッグ表示を全アクターに出すか。エディタが毎フレーム設定し、ゲームの実行時はオフ。
		Bool shapeVisible_ = false;

		/// [EN] Every system the updater runs.
		/// [JP] 更新器が回す全 System。
		Systems systems_;
	};
}
