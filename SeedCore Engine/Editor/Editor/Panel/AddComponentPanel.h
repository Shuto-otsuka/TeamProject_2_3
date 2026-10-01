#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	struct EditorContext;
	class Actor;
	class ImGuiTexture;

	class AddComponentPanel
	{
	public:
		AddComponentPanel(EditorContext& context);

		void Draw(Actor actor, ImGuiTexture& imguiTexture);

	private:
		struct State
		{
			String searchBuffer;
		};

		void DrawSearchBar(ImGuiTexture& imguiTexture);
		void DrawComponentList(Actor actor, ImGuiTexture& imguiTexture);

		/// [EN] Renders one leaf entry as a Selectable, preceded by its
		///      Unity-style icon (ImGuiTexture::ComponentIconType): a single
		///      click adds the component and closes the popup chain.
		///      Already-attached components are shown but disabled.
		/// [JP] 1つの葉ノードを、Unity 風のアイコン
		///      （ImGuiTexture::ComponentIconType）に続けて Selectable として
		///      描画する。1クリックでコンポーネントを追加しポップアップ階層を
		///      閉じる。既に付いているコンポーネントは表示はするが無効化する。
		void DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID, ImGuiTexture& imguiTexture);

		Bool CheckBuiltinComponent(const String& componentName)const;
		Bool CheckFilterMatch(const String& componentName, const std::string& filterText)const;

		EditorContext& context_;

		State state_;
	};
}
