#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class GuizmoPanel2D
	{
	public:
		GuizmoPanel2D(EditorContext& context, ImGuiTexture& imguiTexture);
		~GuizmoPanel2D() = default;

		void Draw(const Vector2& position, const Vector2& size);

		NODISCARD Bool RectToolActive()const;

	private:
		void Move(Matrix& view, Matrix& projection, Matrix& pivot, ImGuizmo::OPERATION operation, const Float* snap);

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		ImGuizmo::MODE currentMode_ = ImGuizmo::WORLD;

		Bool dragActive_ = false;

		DynamicArray<Entity> dragStartEntities_;
		DynamicArray<Matrix> dragStartWorldMatrices_;
		DynamicArray<Vector3> dragStartPositions_;
		DynamicArray<Quaternion> dragStartRotations_;
		DynamicArray<Vector3> dragStartScales_;

		Matrix dragStartPivotMatrix_ = Matrix::Identity;

		Matrix pivotMatrix_ = Matrix::Identity;

		Int32 rectHandle_ = 0;
		Vector4 rectDragStart_ = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
		Vector2 rectDragMouse_ = Vector2(0.0f, 0.0f);
	};
}
