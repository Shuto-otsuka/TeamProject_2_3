#pragma once

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture;
	class AnimatorControllerPanel;
	class AvatarPanel;
	class BenchmarkPanel;
	class BootScreenPanel;
	class CanvasWindowPanel;
	class ConfigPanel;
	class ContentsDrawerPanel;
	class DiagnosticsPanel;
	class EditorWindowPanel;
	class EffectGraphPanel;
	class FullscreenPanel;
	class GameWindowPanel;
	class HierarchyPanel;
	class InspectorPanel;
	class LayerSettingsPanel;
	class MaterialViewerPanel;
	class ModelTransformPanel;
	class ShortCutKeyPanel;
	class SkeletonControllerPanel;
	class SpecMemoPanel;
	class TimelinePanel;
	class TodoListPanel;
	class VersionPanel;

	template<class... Panels>
	class PanelGroup
	{
	public:
		PanelGroup(EditorContext& context, ImGuiTexture& imguiTexture);
		~PanelGroup();

		template<class T>
		NODISCARD T& Panel()
		{
			return *std::get<ResourcePtr<T>>(panels_);
		}

		void Draw();

		Bool Inspect();

	private:
		std::tuple<ResourcePtr<Panels>...> panels_;
	};

	using ToolPanels = 
		PanelGroup
		<
		AnimatorControllerPanel,
		TimelinePanel,
		SkeletonControllerPanel,
		MaterialViewerPanel,
		EffectGraphPanel,
		ModelTransformPanel,
		AvatarPanel,
		BootScreenPanel,
		FullscreenPanel,
		ShortCutKeyPanel,
		SpecMemoPanel,
		TodoListPanel,
		VersionPanel,
		ConfigPanel,
		LayerSettingsPanel,
		BenchmarkPanel
		>;

	using DockPanels =
		PanelGroup
		<
		HierarchyPanel,
		InspectorPanel,
		DiagnosticsPanel,
		EditorWindowPanel,
		CanvasWindowPanel,
		GameWindowPanel,
		ContentsDrawerPanel
		>;
}