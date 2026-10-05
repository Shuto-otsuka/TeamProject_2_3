#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/Renderer/ViewMode.h>

namespace SeedCore
{
	class EditorCamera;
	class EditorCameraController;
	class CanvasCamera;

	/**
	* [EN]
	* The 3D editor view: its camera, how the scene is shown in it, and
	* which helpers are drawn over it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 3D のエディタービュー。カメラ、場面の見せ方、上に重ねて描く補助表示を持つ。
	*/
	struct EditorViewContext
	{
		/// [EN] Camera the editor view is rendered from, owned by Engine.
		/// [JP] エディタービューを描くカメラ。Engine が所有する。
		EditorCamera* camera_ = nullptr;

		/// [EN] Mouse and keyboard control of the camera (fly, orbit, pan, zoom), owned by Engine.
		/// [JP] カメラのマウス・キーボード操作（移動・回り込み・パン・ズーム）。Engine が所有する。
		EditorCameraController* cameraController_ = nullptr;

		/// [EN] How the scene is shown: lit, unlit, wireframe or a buffer visualization.
		/// [JP] 場面の見せ方。ライティングあり・なし、ワイヤーフレーム、各バッファの可視化など。
		ViewMode viewMode_ = ViewMode::Lit;

		/// [EN] Whether icons are drawn at the positions of cameras, lights, audio and other actors without geometry.
		/// [JP] カメラ・ライト・音、そのほか見た目を持たないアクターの位置にアイコンを描くか。
		Bool iconVisible_ = true;

		/// [EN] Whether ranges and shapes (camera frustums, light ranges and so on) are drawn for unselected actors too; selected actors always show theirs.
		/// [JP] 範囲などの形（カメラの視錐台、ライトの届く範囲など）を、選択していないアクターにも描くか。選択中のアクターには常に描く。
		Bool shapeVisible_ = false;
	};

	/**
	* [EN]
	* The 2D canvas view where UI is laid out.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* UI を配置する 2D の Canvas ビュー。
	*/
	struct CanvasViewContext
	{
		/// [EN] Camera the canvas view is rendered from, owned by Engine.
		/// [JP] Canvas ビューを描くカメラ。Engine が所有する。
		CanvasCamera* camera_ = nullptr;
	};

	/**
	* [EN]
	* The views the scene is edited in: the 3D editor view and the 2D
	* canvas view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンを編集するビュー。3D のエディタービューと 2D の Canvas ビュー。
	*/
	struct ViewContext
	{
		/// [EN] The 3D editor view.
		/// [JP] 3D のエディタービュー。
		EditorViewContext editor_;

		/// [EN] The 2D canvas view.
		/// [JP] 2D の Canvas ビュー。
		CanvasViewContext canvas_;
	};
}
