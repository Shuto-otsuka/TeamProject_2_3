#pragma once

namespace SeedCore
{
	struct EditorContext;
	struct Surface;

	class ImGuiTexture;

	class MaterialViewerPanel
	{
	public:
		MaterialViewerPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~MaterialViewerPanel();

		void Draw();

		void Open();

		void Inspect();

		NODISCARD Bool Focused()const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		Uint32 editingAssetID_ = 0;
		ResourcePtr<Surface> editingSurface_;

		Size selectedSlotIndex_ = 0;
	};
}
