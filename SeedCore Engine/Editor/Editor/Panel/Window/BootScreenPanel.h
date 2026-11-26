#pragma once
#include <FoundationEngine/Resource/Config/BootConfig.h>
#include <GraphicsEngine/Screen/Boot/BootTexture.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class BootScreenPanel
	{
	public:
		BootScreenPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~BootScreenPanel() = default;

		void Draw();

		void Inspect();

		void Open();

		NODISCARD Bool Focused()const;

	private:
		void DrawCanvas();

	private:
		enum class GizmoDrag
		{
			None,
			Move,
			TopLeft,
			TopRight,
			BottomLeft,
			BottomRight,
		};

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		BootConfig config_;

		ResourcePtr<BootTexture> texture_;

		Uint32 screenWidth_ = 1920;
		Uint32 screenHeight_ = 1080;

		Bool imagesDirty_ = true;
		Bool saveRequested_ = false;

		Float progress_ = 0.6f;
		Bool autoPlay_ = true;

		Float zoom_ = 1.0f;
		Vector2 pan_ = Vector2(0.0f, 0.0f);

		GizmoDrag dragHandle_ = GizmoDrag::None;
		Vector2 dragStartMouse_ = { 0.0f, 0.0f };
		Vector2 dragStartOffset_ = { 0.0f, 0.0f };
		Vector4 dragStartRect_ = { 0.0f, 0.0f, 0.0f, 0.0f };
	};
}
