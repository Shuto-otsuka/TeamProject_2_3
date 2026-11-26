#pragma once

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture;

	template<class T>
	concept EmbedPanel = std::constructible_from<T, EditorContext&, ImGuiTexture&> && requires { &T::Draw; };

	template<class T>
	concept WindowPanel = std::constructible_from<T, EditorContext&, ImGuiTexture&> && requires(T& panel) 
	{
		panel.Draw(); 
	};

	template<class T>
	concept InspectPanel = WindowPanel<T> && requires(T& panel, const T& constPanel)
	{
		panel.Inspect();
		{ constPanel.Focused() } -> std::same_as<Bool>;
	};
}