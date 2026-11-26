#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class Crister;

	class SkeletonControllerPanel
	{
	public:
		SkeletonControllerPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~SkeletonControllerPanel() = default;

		void Draw();

		void Open();

		void Inspect();

		NODISCARD Bool Focused()const;

	private:
		void DrawBoneList(const Crister& crister);

		void DrawPreview();

	private:
		SC_CONSTEX Float boneTreeColumnWidthRatio_ = 0.2f;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		Int selectedNodeIndex_ = -1;

		Crister* currentCrister_ = nullptr;
	};
}
