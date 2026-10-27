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

		/// [EN] State taken when Play starts and restored when it stops, so a play session leaves the edited scene unchanged: the scene's actors and the mixer volumes.
		/// [JP] Play 開始時に取っておき、停止時に戻す状態。Play しても編集中のシーンが変わらないようにする。シーンのアクターとミキサーの音量。
		SceneBlueprint playModeScene_;
		Float playModeMasterVolume_ = 1.0f;
		DynamicArray<Float> playModeCategoryVolumes_;
	};
}
