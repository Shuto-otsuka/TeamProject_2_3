#pragma once
#include <FoundationEngine/Resource/Asset/AxisConvention.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;

	class ModelTransformPanel
	{
	public:
		ModelTransformPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ModelTransformPanel() = default;

		void Draw();

		void Open();

		void Open(Uint32 assetId);

	private:
		void SetTarget(Uint32 assetId);

		void DrawPreview();

		void DrawAxisInspector(Uint32 assetId);

		void ApplyConversion(Uint32 assetId);

		void DrawBaseTransformInspector(Uint32 assetId);

		void ApplyTransformConversion(Uint32 assetId);

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		Uint32 targetMeshAssetID_ = 0;

		Uint32 lastSelectionMeshID_ = 0;

		AxisConvention editConvention_;

		Vector3 baseTransformPosition_ = { 0.0f, 0.0f, 0.0f };
		Vector3 baseTransformRotation_ = { 0.0f, 0.0f, 0.0f };
		Vector3 baseTransformScale_ = { 1.0f, 1.0f, 1.0f };
		Vector3 baseTransformPivot_ = { 0.0f, 0.0f, 0.0f };

		ImGuizmo::OPERATION baseTransformGizmoOperation_ = ImGuizmo::TRANSLATE;
	};
}
