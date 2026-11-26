#pragma once
#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ControlPanel
	{
	public:
		ControlPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ControlPanel() = default;

		Float Draw();

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		SceneBlueprint playModeScene_;

		Float playModeMasterVolume_ = 1.0f;
		DynamicArray<Float> playModeCategoryVolumes_;
	};
}
