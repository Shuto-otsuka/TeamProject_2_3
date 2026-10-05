#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/Renderer/ViewMode.h>
#include <Editor/Editor/Panel/GraphicsMenuPanel.h>
#include <Editor/Editor/Build/RuntimeBuilder.h>

namespace SeedCore
{
	struct EditorContext;

	class MenuBarPanel
	{
	public:
		MenuBarPanel(EditorContext& context);
		~MenuBarPanel() = default;

		void Draw();

	private:
		enum class PendingSceneOp
		{
			None,
			New,
			OpenPath,
			OpenAsset,
			Exit,
		};

		void BuildRuntime();

		void RequestSceneSwitch(PendingSceneOp op, const std::filesystem::path& path, Uint32 assetID);

		void ExecutePendingSceneOp();

		void NewScene();

		void OpenScene();

		void SaveScene();

		void OverwriteSaveScene();

	private:
		EditorContext& context_;

		GraphicsMenuPanel graphicsMenuPanel_;
		RuntimeBuilder runtimeBuilder_;

		PendingSceneOp pendingSceneOp_ = PendingSceneOp::None;
		std::filesystem::path pendingScenePath_;
		Uint32 pendingSceneAssetID_ = 0;
	};
}
