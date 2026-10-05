#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Command/History.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <Editor/Editor/Context/ApplicationContext.h>
#include <Editor/Editor/Context/WorldContext.h>
#include <Editor/Editor/Context/GraphicsContext.h>
#include <Editor/Editor/Context/GuizmoContext.h>
#include <Editor/Editor/Context/SceneContext.h>
#include <Editor/Editor/Context/SceneVisualContext.h>
#include <Editor/Editor/Context/SelectionContext.h>
#include <Editor/Editor/Context/ViewContext.h>
#include <Editor/Editor/Context/ConfigContext.h>
#include <Editor/Editor/Context/PreviewContext.h>
#include <Editor/Editor/Context/PanelContext.h>
#include <GraphicsEngine/Renderer/ViewMode.h>
#include <GraphicsEngine/Raytracing/RaytracingContext.h>
#include <GraphicsEngine/ScreenSpace/ScreenSpaceContext.h>
#include <GraphicsEngine/Rasterization/RasterizationContext.h>
#include <GraphicsEngine/Quality/GraphicsQuality.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>

namespace SeedCore
{
	/**
	* [EN]
	* Everything the editor's panels share, passed to each panel by
	* reference. Each member groups one area of the editor; the services in
	* it are owned by Engine or Editor, not by this struct.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタの各パネルが共有するものすべて。各パネルへ参照で渡す。
	* メンバーはエディタの領域ごとのまとまりで、中のサービスはこの構造体では
	* なく Engine や Editor が所有する。
	*/
	struct EditorContext
	{
		/// [EN] Editor-wide state: shared-asset sync, UI frame counter, quit request.
		/// [JP] エディタ全体の状態。共有アセットの同期、UI のフレーム番号、終了の要求。
		ApplicationContext application_;

		/// [EN] The world being edited and the services that load into and run it.
		/// [JP] 編集中のワールドと、そこへの読み込みや実行を担うサービス。
		WorldContext world_;

		/// [EN] The renderer and the ImGui backend.
		/// [JP] レンダラーと ImGui のバックエンド。
		GraphicsContext graphics_;

		/// [EN] Transform gizmo settings, shared by the editor view and the canvas view.
		/// [JP] トランスフォームギズモの設定。エディタービューと Canvas ビューで共有する。
		GuizmoContext guizmo_;

		/// [EN] The scene being edited: its file, a request to open another, and the undo/redo history.
		/// [JP] 編集中のシーン。ファイル、別のシーンを開く依頼、元に戻す・やり直しの履歴。
		SceneContext scene_;

		/// [EN] Rendering settings saved with the scene, until they become components.
		/// [JP] シーンと一緒に保存する描画の設定。コンポーネントになるまでのつなぎ。
		SceneVisualContext sceneVisual_;

		/// [EN] Selected actors; the last one is the primary selection.
		/// [JP] 選択中のアクター。最後の1つが主な選択。
		SelectionContext selection_;

		/// [EN] The views the scene is edited in: the 3D editor view and the 2D canvas view.
		/// [JP] シーンを編集するビュー。3D のエディタービューと 2D の Canvas ビュー。
		ViewContext view_;

		/// [EN] The game's and the editor's settings files shared with Engine, and the requests to rebuild rendering after a game setting changes.
		/// [JP] Engine と共有するゲームとエディタの設定ファイルと、ゲームの設定が変わったあとに描画を作り直す依頼。
		ConfigContext config_;

		/// [EN] The previews drawn in the tool panels (timeline, model transform, material, skeleton, avatar, boot screen).
		/// [JP] ツールパネルに出すプレビュー（タイムライン、モデル変換、マテリアル、スケルトン、アバター、起動画面）。
		PreviewContext preview_;

		/// [EN] Panels that other panels open or ask to draw their details in the inspector.
		/// [JP] ほかのパネルから開いたり、インスペクターに詳細を描かせたりするパネル。
		PanelContext panel_;
	};
}
