#pragma once
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ResourceSyncControlPanel
	{
	public:
		ResourceSyncControlPanel(EditorContext& context, ImGuiTexture& imguiTexture);

		void Draw();

		void DrawActions(const AssetRecord& asset);

		void DrawState(const AssetRecord& asset);

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;
	};
}
