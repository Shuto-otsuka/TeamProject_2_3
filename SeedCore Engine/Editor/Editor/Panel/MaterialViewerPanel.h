#pragma once

namespace SeedCore
{
	struct EditorContext;
	struct Surface;

	class ImGuiTexture;

	/**
	* [EN]
	* Material Viewer: a 3D preview of one of the selected actor's material
	* slots on that actor's mesh, with an orbit camera and a slot combo. The
	* Surface parameter editor (shading model, KHR, save) is drawn into the
	* shared Inspector window via DrawDetails() while this panel is focused -
	* same pattern as TimelinePanel/AnimatorControllerPanel.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マテリアルビューア: 選択アクターのマテリアルスロット1つを、その
	* アクターのメッシュ上でオービットカメラ + スロットコンボ付き3D
	* プレビューする。Surface パラメータエディタ(シェーディングモデル/
	* KHR/保存)は、このパネルがフォーカス中の間 DrawDetails() で共有
	* Inspector ウィンドウへ描画する - TimelinePanel/AnimatorControllerPanel
	* と同じ方式。
	*/
	class MaterialViewerPanel
	{
	public:
		MaterialViewerPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~MaterialViewerPanel();

		void Draw();

		void Open();

		void DrawDetails();

		NODISCARD Bool Visible()const;

	private:
		struct EditingState
		{
			Uint32 assetID_ = 0;
			ResourcePtr<Surface> surface_;
		};

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		EditingState editingState_;

		Bool show_ = false;

		Bool visible_ = false;

		Size selected_ = 0;
	};
}
