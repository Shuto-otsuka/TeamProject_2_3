#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	struct LibraryEntry
	{
		const Char* name_;
		const Char* version_;
		const Char* usage_;
	};

	class SpecMemoPanel
	{
	public:
		SpecMemoPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~SpecMemoPanel() = default;

		void Draw();

		void Open();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;
	};
}
