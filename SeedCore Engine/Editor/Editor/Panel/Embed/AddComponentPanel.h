#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	struct EditorContext;

	class Actor;
	class ImGuiTexture;

	class AddComponentPanel
	{
	public:
		AddComponentPanel(EditorContext& context, ImGuiTexture& imguiTexture);

		void Draw(Actor actor);

	private:
		void DrawSearchBar();

		void DrawComponent(Actor actor);

		void DrawCategory(Actor actor, const std::string& path);

		void DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID);

	private:
		Bool CheckBuiltin(const String& componentName)const;

		Bool CheckFilter(const String& componentName, const std::string& filterText)const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		String search_;

		DynamicArray<std::pair<String, ComponentID>> matches_;
	};
}
