#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	struct BootConfig;

	class AvatarMesh;
	class BootScreenRenderer;
	class PreviewCamera;
	class PreviewCameraController;

	/**
	* [EN]
	* The timeline preview: the selected actor's mesh played with the chosen
	* animation at the scrubbed time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タイムラインのプレビュー。選択中のアクターのメッシュを、選んだアニメーションの
	* 再生位置で見せる。
	*/
	struct TimelinePreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only while there is something to show.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、見せるものがあるときだけ立てる。
		Bool active_ = false;

		/// [EN] Mesh of the selected actor.
		/// [JP] 選択中のアクターのメッシュ。
		Uint32 meshAssetID_ = 0;

		/// [EN] Animation chosen in the panel, 0 when none is chosen.
		/// [JP] パネルで選んだアニメーション。未選択なら 0。
		Uint32 animationAssetID_ = 0;

		/// [EN] Playback time in seconds after the animation's speed curve is applied.
		/// [JP] アニメーションの速度カーブを通したあとの再生位置（秒）。
		Float time_ = 0.0f;

		/// [EN] Camera this preview is rendered from, owned by Engine.
		/// [JP] このプレビューを描くカメラ。Engine が所有する。
		PreviewCamera* camera_ = nullptr;

		/// [EN] Orbit, pan and dolly control of the camera, owned by Engine.
		/// [JP] カメラの回り込み・パン・前後移動の操作。Engine が所有する。
		PreviewCameraController* cameraController_ = nullptr;
	};

	/**
	* [EN]
	* The model transform preview: a model shown with the axis and base
	* transform being edited, exactly as applying them would bake it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデル変換のプレビュー。編集中の軸と基準のトランスフォームを、適用したときと
	* 同じ結果でモデルに掛けて見せる。
	*/
	struct ModelTransformPreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only while the model is loaded.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、モデルを読み込み済みのときだけ立てる。
		Bool active_ = false;

		/// [EN] Model being converted.
		/// [JP] 変換の対象のモデル。
		Uint32 meshAssetID_ = 0;

		/// [EN] Pivot, position, rotation and scale being edited, combined into one matrix.
		/// [JP] 編集中のピボット・位置・回転・拡縮をまとめた行列。
		Matrix worldMatrix_ = Matrix::Identity;

		/// [EN] Camera this preview is rendered from, owned by Engine.
		/// [JP] このプレビューを描くカメラ。Engine が所有する。
		PreviewCamera* camera_ = nullptr;

		/// [EN] Orbit, pan and dolly control of the camera, owned by Engine.
		/// [JP] カメラの回り込み・パン・前後移動の操作。Engine が所有する。
		PreviewCameraController* cameraController_ = nullptr;
	};

	/**
	* [EN]
	* The material viewer preview: the selected actor's mesh drawn with one
	* of its material slots.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マテリアルビューアのプレビュー。選択中のアクターのメッシュを、選んだ
	* マテリアルスロットで描く。
	*/
	struct MaterialPreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only while an actor with a material and a mesh is selected.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、マテリアルとメッシュを持つアクターを選んでいるときだけ立てる。
		Bool active_ = false;

		/// [EN] Mesh of the selected actor.
		/// [JP] 選択中のアクターのメッシュ。
		Uint32 meshAssetID_ = 0;

		/// [EN] Material in the chosen slot; the panel also reads it back to pick the material it edits.
		/// [JP] 選んだスロットのマテリアル。パネルも編集するマテリアルを決めるために読み返す。
		Uint32 surfaceAssetID_ = 0;

		/// [EN] Camera this preview is rendered from, owned by Engine.
		/// [JP] このプレビューを描くカメラ。Engine が所有する。
		PreviewCamera* camera_ = nullptr;

		/// [EN] Orbit, pan and dolly control of the camera, owned by Engine.
		/// [JP] カメラの回り込み・パン・前後移動の操作。Engine が所有する。
		PreviewCameraController* cameraController_ = nullptr;
	};

	/**
	* [EN]
	* The skeleton controller preview: the selected actor's skeletal mesh
	* with the chosen joint highlighted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スケルトンコントローラーのプレビュー。選択中のアクターのスケルタルメッシュを、
	* 選んだジョイントを強調して見せる。
	*/
	struct SkeletonControllerPreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only while the model is loaded.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、モデルを読み込み済みのときだけ立てる。
		Bool active_ = false;

		/// [EN] Mesh of the selected actor.
		/// [JP] 選択中のアクターのメッシュ。
		Uint32 meshAssetID_ = 0;

		/// [EN] Node index of the chosen joint, -1 for the whole rig.
		/// [JP] 選んだジョイントのノード番号。-1 ならリグ全体。
		Int nodeIndex_ = -1;

		/// [EN] Camera this preview is rendered from and joints are picked through, owned by Engine.
		/// [JP] このプレビューを描き、ジョイントを選ぶレイを飛ばすカメラ。Engine が所有する。
		PreviewCamera* camera_ = nullptr;

		/// [EN] Orbit, pan and dolly control of the camera, owned by Engine.
		/// [JP] カメラの回り込み・パン・前後移動の操作。Engine が所有する。
		PreviewCameraController* cameraController_ = nullptr;
	};

	/**
	* [EN]
	* The avatar preview: the generated human or animal body with the
	* current shape and region textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アバター生成のプレビュー。生成した人間か動物の体を、今の形と部位ごとの
	* テクスチャで見せる。
	*/
	struct AvatarPreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only once the mesh is created.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、メッシュを作成済みのときだけ立てる。
		Bool active_ = false;

		/// [EN] Mesh owned by the panel; Engine uploads the vertices below into it before rendering.
		/// [JP] パネルが所有するメッシュ。Engine が描く前に下の頂点を書き込む。
		AvatarMesh* mesh_ = nullptr;

		/// [EN] Vertex positions evaluated from the current shape.
		/// [JP] 今の形から計算した頂点の位置。
		std::span<const Vector3> positions_;

		/// [EN] Vertex normals evaluated from the current shape.
		/// [JP] 今の形から計算した頂点の法線。
		std::span<const Vector3> normals_;

		/// [EN] Number of bones in the model.
		/// [JP] モデルのボーン数。
		Uint32 boneCount_ = 0;

		/// [EN] Bindless texture index for each body region, 0xFFFFFFFF where no texture is set.
		/// [JP] 部位ごとのテクスチャの bindless インデックス。未設定なら 0xFFFFFFFF。
		Uint32 regionTextureIndices_[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

		/// [EN] Number of regions the current model uses.
		/// [JP] 今のモデルが使う部位の数。
		Uint32 regionCount_ = 0;

		/// [EN] Camera this preview is rendered from, owned by Engine.
		/// [JP] このプレビューを描くカメラ。Engine が所有する。
		PreviewCamera* camera_ = nullptr;

		/// [EN] Orbit, pan and dolly control of the camera, owned by Engine.
		/// [JP] カメラの回り込み・パン・前後移動の操作。Engine が所有する。
		PreviewCameraController* cameraController_ = nullptr;
	};

	/**
	* [EN]
	* The boot screen preview: the loading screen being edited, drawn at
	* the given progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 起動ローディング画面のプレビュー。編集中のローディング画面を、指定した
	* 進み具合で描く。
	*/
	struct BootScreenPreviewContext
	{
		/// [EN] Whether Engine renders this preview this frame; the panel clears it every frame and sets it only while its window is open.
		/// [JP] このフレームに Engine がこのプレビューを描くか。パネルが毎フレーム下ろし、ウィンドウが開いているときだけ立てる。
		Bool active_ = false;

		/// [EN] Renderer owned by the panel that draws the loading screen.
		/// [JP] ローディング画面を描くレンダラー。パネルが所有する。
		BootScreenRenderer* renderer_ = nullptr;

		/// [EN] Loading screen settings being edited, owned by the panel.
		/// [JP] 編集中のローディング画面の設定。パネルが所有する。
		const BootConfig* config_ = nullptr;

		/// [EN] Loading progress from 0 to 1.
		/// [JP] 読み込みの進み具合（0〜1）。
		Float progress_ = 0.0f;
	};

	/**
	* [EN]
	* The previews drawn in the editor's tool panels. Each panel fills its
	* own entry while it draws, and Engine renders the active ones into
	* their own render targets.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタのツールパネルに出すプレビュー。各パネルが描画中に自分の分を埋め、
	* Engine が立っているものをそれぞれの描画先へ描く。
	*/
	struct PreviewContext
	{
		/// [EN] Timeline preview.
		/// [JP] タイムラインのプレビュー。
		TimelinePreviewContext timeline_;

		/// [EN] Model transform preview.
		/// [JP] モデル変換のプレビュー。
		ModelTransformPreviewContext modelTransform_;

		/// [EN] Material viewer preview.
		/// [JP] マテリアルビューアのプレビュー。
		MaterialPreviewContext material_;

		/// [EN] Skeleton controller preview.
		/// [JP] スケルトンコントローラーのプレビュー。
		SkeletonControllerPreviewContext skeletonController_;

		/// [EN] Avatar preview.
		/// [JP] アバター生成のプレビュー。
		AvatarPreviewContext avatar_;

		/// [EN] Boot screen preview.
		/// [JP] 起動ローディング画面のプレビュー。
		BootScreenPreviewContext bootScreen_;
	};
}
