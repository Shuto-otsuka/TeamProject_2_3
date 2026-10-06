#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <Editor/Editor/Panel/GuizmoPanel3D.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class EditorWindowPanel
	{
	public:
		EditorWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~EditorWindowPanel() = default;

		void Draw(D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle);

	private:
		void DrawGuizmo();

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
			IconType type_ = IconType::Actor;
		};

	private:
		SC_CONST Float iconSize_ = 28.0f;

		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		GuizmoPanel3D guizmoPanel_;

		DynamicArray<ViewIcon> icons_;
	};
}
