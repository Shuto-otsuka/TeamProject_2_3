#pragma once

namespace SeedCore
{
	struct GuizmoContext
	{
		Bool visible_ = true;

		ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;

		Bool pivotEdit_ = false;

		Bool rectTool_ = false;

		Float translateSnap_ = 1.0f;

		Float rotateSnap_ = 5.0f;

		Float scaleSnap_ = 0.5f;
	};
}
