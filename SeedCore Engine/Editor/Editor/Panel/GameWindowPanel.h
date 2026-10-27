#pragma once
#include <Editor/Editor/ImGui/ImGuiTexture.h>

namespace SeedCore
{
	struct EditorContext;

	class GameWindowPanel
	{
	public:
		GameWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~GameWindowPanel() = default;

		void Draw(D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle, Float toolbarHeight);

		Bool Fullscreen()const { return fullscreen_; }

		Bool ImageHovered()const { return imageHovered_; }

	private:
		EditorContext& context_;
		ImGuiTexture& imguiTexture_;
		Bool fullscreen_ = false;
		Bool imageHovered_ = false;
	};
}
