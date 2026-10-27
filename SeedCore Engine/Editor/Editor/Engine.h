#pragma once
#include <SeedCore/Engine/EngineCore.h>

#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>
#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/Editor.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/Build/HotReload.h>

namespace SeedCore
{
	struct Bootstrap;

	class Engine :public NonTransferable
	{
	public:
		Engine() = default;
		~Engine() = default;

		Bool Boot(const Bootstrap& bootstrap);

		void Run();

		void Shutdown();

	private:
		EngineCore core_;

		EditorConfig editorConfig_;

		GameConfig gameConfig_;

		ResourcePtr<Editor> editor_;

		ResourcePtr<ImGuiRenderer> imgui_;

		HotReload hotReload_;

		EditorContext editorContext_;

		EditorCamera editorCamera_;

		EditorCameraController editorCameraController_;

		CanvasCamera canvasCamera_;
	};
}
