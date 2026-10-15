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
		/// [EN] Whether the gizmo is shown at all; off means plain selection with no handles.
		/// [JP] ギズモを表示するか。オフのときはハンドルなしの選択だけになる。
		Bool visible_ = true;

		/// [EN] Which handles the gizmo shows: move, rotate or scale.
		/// [JP] ギズモが出すハンドル。移動・回転・拡大縮小のどれか。
		ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;

		/// [EN] Whether the pivot tool is selected: its move handles edit the pivot point instead of the actor, so the actor stays where it is on screen while its Pivot and Position move together.
		/// [JP] 基準点ツールを選んでいるか。移動ハンドルで actor ではなく基準点を編集し、actor は画面上でその場に留まったまま Pivot と Position が一緒に動く。
		Bool pivotEdit_ = false;

		/// [EN] Whether the rect tool is selected, replacing the move/rotate/scale handles.
		/// [JP] 矩形ツールを選んでいるか。移動・回転・拡大縮小のハンドルの代わりになる。
		Bool rectTool_ = false;

		/// [EN] Snap step for moving, in world units.
		/// [JP] 移動のスナップ量（ワールド単位）。
		Float translateSnap_ = 1.0f;

		/// [EN] Snap step for rotating, in degrees.
		/// [JP] 回転のスナップ量（度）。
		Float rotateSnap_ = 5.0f;

		/// [EN] Snap step for scaling.
		/// [JP] 拡大縮小のスナップ量。
		Float scaleSnap_ = 0.5f;
	};
}
