#pragma once
#include <FoundationEngine/World/Actor/Actor.h>
#include <Editor/Editor/Panel/Embed/GuizmoPanel3D.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class EditorViewPanel
	{
	public:
		EditorViewPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~EditorViewPanel() = default;

		void Draw();

	private:
		void DrawManipulator(const Vector2& position, const Vector2& size);

		void DrawIcon(const Vector2& position, const Vector2& size);

		void DrawSpeed(const Vector2& position);

	private:
		void UpdatePick(const Vector2& position, const Vector2& size);

		void UpdateSnap();

		void UpdateCamera();

	private:
		struct ViewIcon
		{
			Actor actor_;
			Vector2 center_ = { 0.0f,0.0f };
			Color color_ = { 1.0f,1.0f,1.0f,1.0f };
			Float depth_ = 0.0f;
			String type_ = String("Hierarchy/Actor");
		};

	private:
		SC_CONSTEX Float iconSize_ = 28.0f;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		GuizmoPanel3D guizmoPanel_;

		DynamicArray<ViewIcon> icons_;
	};
}
