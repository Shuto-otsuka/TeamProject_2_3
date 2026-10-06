#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Settings of the transform gizmo used to move, rotate and scale the
	* selection, shared by the editor view and the canvas view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 選択中のものを移動・回転・拡大縮小するトランスフォームギズモの設定。
	* エディタービューと Canvas ビューで共有する。
	*/
	struct GuizmoContext
	{
		/// [EN] Snap step for moving, in world units.
		/// [JP] 移動のスナップ量（ワールド単位）。
		Float translateSnap_ = 1.0f;

		/// [EN] Snap step for rotating, in degrees.
		/// [JP] 回転のスナップ量（度）。
		Float rotateSnap_ = 5.0f;

		/// [EN] Snap step for scaling.
		/// [JP] 拡大縮小のスナップ量。
		Float scaleSnap_ = 0.5f;

		/// [EN] Whether the gizmo is shown at all; off means plain selection with no handles.
		/// [JP] ギズモを表示するか。オフのときはハンドルなしの選択だけになる。
		Bool visible_ = true;

		/// [EN] Whether the rect tool replaces the move/rotate/scale handles.
		/// [JP] 移動・回転・拡大縮小のハンドルの代わりに矩形ツールを使うか。
		Bool rectTool_ = false;

		/// [EN] Which handles the gizmo shows: move, rotate or scale.
		/// [JP] ギズモが出すハンドル。移動・回転・拡大縮小のどれか。
		ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;
	};
}
