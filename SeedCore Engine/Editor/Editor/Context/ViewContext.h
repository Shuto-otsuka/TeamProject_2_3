#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	class EditorCamera;
	class EditorCameraController;
	class CanvasCamera;
	class CameraSystem;

	enum class FocusedView
	{
		Editor,
		Game,
		Canvas
	};

	struct EditorViewContext
	{
		EditorCamera* camera_ = nullptr;

		EditorCameraController* cameraController_ = nullptr;

		RenderViewMode viewMode_ = RenderViewMode::Lit;

		RenderDebugSource debugSource_ = RenderDebugSource::Reflection;

		Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::All);

		Bool iconVisible_ = true;

		Bool shapeVisible_ = false;
	};

	struct GameViewContext
	{
		CameraSystem* cameraSystem_ = nullptr;
	};

	struct CanvasViewContext
	{
		CanvasCamera* camera_ = nullptr;
	};

	struct ViewContext
	{
		EditorViewContext editor_;

		GameViewContext game_;

		CanvasViewContext canvas_;

		FocusedView focused_ = FocusedView::Editor;
	};
}
