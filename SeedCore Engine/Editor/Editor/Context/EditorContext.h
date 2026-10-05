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
#include <GraphicsEngine/Renderer/ViewMode.h>
#include <GraphicsEngine/Raytracing/RaytracingContext.h>
#include <GraphicsEngine/ScreenSpace/ScreenSpaceContext.h>
#include <GraphicsEngine/Rasterization/RasterizationContext.h>
#include <GraphicsEngine/Quality/GraphicsQuality.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>

namespace SeedCore
{
	class PreviewCamera;
	class PreviewCameraController;
	class CameraSystem;
	class AnimatorControllerPanel;
	class TimelinePanel;
	class LayerSettingsPanel;
	class MaterialViewerPanel;
	class SkeletonControllerPanel;
	class AvatarPanel;
	class BootScreenPanel;
	class AvatarMesh;
	class BootScreenRenderer;
	struct BootConfig;

	struct CameraContext
	{
		PreviewCamera* timelineCamera_ = nullptr;
		PreviewCamera* modelTransformCamera_ = nullptr;
		PreviewCamera* materialCamera_ = nullptr;
		PreviewCamera* skeletonControllerCamera_ = nullptr;
		PreviewCamera* avatarCamera_ = nullptr;
		PreviewCameraController* timelineCameraController_ = nullptr;
		PreviewCameraController* modelTransformCameraController_ = nullptr;
		PreviewCameraController* materialCameraController_ = nullptr;
		PreviewCameraController* skeletonControllerCameraController_ = nullptr;
		PreviewCameraController* avatarCameraController_ = nullptr;
		CameraSystem* cameraSystem_ = nullptr;
	};

	struct TimelinePreviewContext
	{
		Bool previewActive_ = false;
		Uint32 previewMeshAssetId_ = 0;
		Uint32 previewAnimationAssetId_ = 0;
		Float previewTime_ = 0.0f;
	};

	struct ModelTransformPreviewContext
	{
		Bool previewActive_ = false;
		Uint32 previewMeshAssetId_ = 0;
		Matrix previewWorldMatrix_ = Matrix::Identity;
		Uint32 requestedAssetId_ = 0;
	};

	struct MaterialPreviewContext
	{
		Bool previewActive_ = false;
		Uint32 previewMeshAssetId_ = 0;
		Uint32 previewSurfaceAssetId_ = 0;
		Matrix previewWorldMatrix_ = Matrix::Identity;
	};

	struct SkeletonControllerPreviewContext
	{
		Bool previewActive_ = false;
		Uint32 previewMeshAssetId_ = 0;
		Matrix previewWorldMatrix_ = Matrix::Identity;
		Int selectedNodeIndex_ = -1;
	};

	struct AvatarPreviewContext
	{
		Bool previewActive_ = false;
		AvatarMesh* mesh_ = nullptr;
		std::span<const Vector3> positions_;
		std::span<const Vector3> normals_;
		Uint32 boneCount_ = 0;
		Matrix previewWorldMatrix_ = Matrix::Identity;
		Uint32 regionTextureIndices_[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		Uint32 regionCount_ = 0;
	};

	struct BootScreenPreviewContext
	{
		Bool previewActive_ = false;
		BootScreenRenderer* renderer_ = nullptr;
		const BootConfig* config_ = nullptr;
		Float progress_ = 0.0f;
	};

	struct PanelContext
	{
		AnimatorControllerPanel* animatorControllerPanel_ = nullptr;
		TimelinePanel* timelinePanel_ = nullptr;
		LayerSettingsPanel* layerSettingsPanel_ = nullptr;
		MaterialViewerPanel* materialViewerPanel_ = nullptr;
		SkeletonControllerPanel* skeletonControllerPanel_ = nullptr;
		AvatarPanel* avatarPanel_ = nullptr;
		BootScreenPanel* bootScreenPanel_ = nullptr;
	};

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

		/// [EN] The game's settings file shared with Engine, and the requests to rebuild rendering after it changes.
		/// [JP] Engine と共有するゲームの設定ファイルと、変更後に描画を作り直す依頼。
		ConfigContext config_;

		CameraContext cameraContext_;
		TimelinePreviewContext timelinePreviewContext_;
		ModelTransformPreviewContext modelTransformPreviewContext_;
		MaterialPreviewContext materialPreviewContext_;
		SkeletonControllerPreviewContext skeletonControllerPreviewContext_;
		AvatarPreviewContext avatarPreviewContext_;
		BootScreenPreviewContext bootScreenPreviewContext_;
		PanelContext panelContext_;
	};
}
