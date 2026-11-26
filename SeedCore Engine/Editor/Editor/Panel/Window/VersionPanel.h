#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class VersionPanel
	{
	public:
		VersionPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~VersionPanel() = default;

		void Draw();

		void Open();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		ImTextureID logoTextureId_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> logoResource_;

		Float logoWidth_ = 0.0f;
		Float logoHeight_ = 0.0f;

		String gpuName_;
		String osInfo_;
		String memoryInfo_;
	};
}
