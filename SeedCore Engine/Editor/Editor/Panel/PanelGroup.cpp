#include <Editor/Editor/Panel/PanelGroup.h>
#include <Editor/Editor/Panel/Panel.h>
#include <Editor/Editor/Panel/Embed/AddComponentPanel.h>
#include <Editor/Editor/Panel/Embed/CanvasViewPanel.h>
#include <Editor/Editor/Panel/Embed/ConsolePanel.h>
#include <Editor/Editor/Panel/Embed/EditorViewPanel.h>
#include <Editor/Editor/Panel/Embed/GameViewPanel.h>
#include <Editor/Editor/Panel/Embed/GraphicsMenuPanel.h>
#include <Editor/Editor/Panel/Embed/GuizmoPanel2D.h>
#include <Editor/Editor/Panel/Embed/GuizmoPanel3D.h>
#include <Editor/Editor/Panel/Embed/ResourceSyncControlPanel.h>
#include <Editor/Editor/Panel/Embed/ViewModePanel.h>
#include <Editor/Editor/Panel/Window/AnimatorControllerPanel.h>
#include <Editor/Editor/Panel/Window/AvatarPanel.h>
#include <Editor/Editor/Panel/Window/BenchmarkPanel.h>
#include <Editor/Editor/Panel/Window/BootScreenPanel.h>
#include <Editor/Editor/Panel/Window/CanvasWindowPanel.h>
#include <Editor/Editor/Panel/Window/ConfigPanel.h>
#include <Editor/Editor/Panel/Window/ContentsDrawerPanel.h>
#include <Editor/Editor/Panel/Window/ControlPanel.h>
#include <Editor/Editor/Panel/Window/DiagnosticsPanel.h>
#include <Editor/Editor/Panel/Window/EditorWindowPanel.h>
#include <Editor/Editor/Panel/Window/EffectGraphPanel.h>
#include <Editor/Editor/Panel/Window/FullscreenPanel.h>
#include <Editor/Editor/Panel/Window/GameWindowPanel.h>
#include <Editor/Editor/Panel/Window/HierarchyPanel.h>
#include <Editor/Editor/Panel/Window/InspectorPanel.h>
#include <Editor/Editor/Panel/Window/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/Window/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/Window/MenuBarPanel.h>
#include <Editor/Editor/Panel/Window/ModelTransformPanel.h>
#include <Editor/Editor/Panel/Window/ShortCutKeyPanel.h>
#include <Editor/Editor/Panel/Window/SkeletonControllerPanel.h>
#include <Editor/Editor/Panel/Window/SpecMemoPanel.h>
#include <Editor/Editor/Panel/Window/TimelinePanel.h>
#include <Editor/Editor/Panel/Window/TodoListPanel.h>
#include <Editor/Editor/Panel/Window/VersionPanel.h>
#include <FoundationEngine/Benchmark/Benchmark.h>

namespace SeedCore
{
	SC_STATIC_ASSERT(EmbedPanel<AddComponentPanel> && EmbedPanel<CanvasViewPanel> && EmbedPanel<ConsolePanel> && EmbedPanel<EditorViewPanel> && EmbedPanel<GameViewPanel> && EmbedPanel<GraphicsMenuPanel> && EmbedPanel<GuizmoPanel2D> && EmbedPanel<GuizmoPanel3D> && EmbedPanel<ResourceSyncControlPanel> && EmbedPanel<ViewModePanel>, "Embed panel does not match EmbedPanel");

	template<class... Panels>
	PanelGroup<Panels...>::PanelGroup(EditorContext& context, ImGuiTexture& imguiTexture) : panels_(MakePtr<Panels>(context, imguiTexture)...)
	{
		SC_STATIC_ASSERT((WindowPanel<Panels> && ...), "Panel group member does not match WindowPanel");
	}

	template<class... Panels>
	PanelGroup<Panels...>::~PanelGroup() = default;

	template<class... Panels>
	void PanelGroup<Panels...>::Draw()
	{
		([this]()
			{
				static const String name = []()
					{
						std::string_view typeName = typeid(Panels).name();
						return String(typeName.substr(typeName.rfind(':') + 1));
					}();
				Benchmark::Begin(name);
				Panel<Panels>().Draw();
				Benchmark::End(name);
			}(), ...);
	}

	template<class... Panels>
	Bool PanelGroup<Panels...>::Inspect()
	{
		return ([this]()
			{
				if constexpr (InspectPanel<Panels>)
				{
					if (Panel<Panels>().Focused())
					{
						Panel<Panels>().Inspect();
						return true;
					}
				}
				return false;
			}() || ...);
	}

	template ToolPanels::PanelGroup(EditorContext& context, ImGuiTexture& imguiTexture);

	template ToolPanels::~PanelGroup();

	template void ToolPanels::Draw();

	template Bool ToolPanels::Inspect();

	template DockPanels::PanelGroup(EditorContext& context, ImGuiTexture& imguiTexture);

	template DockPanels::~PanelGroup();

	template void DockPanels::Draw();

	template Bool DockPanels::Inspect();
}