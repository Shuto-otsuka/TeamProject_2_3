#include <Editor/Editor/Panel/Window/FullscreenPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>

namespace SeedCore
{
	FullscreenPanel::FullscreenPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), gameViewPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void FullscreenPanel::Draw()
	{
		if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_F11))
		{
			open_ = !open_;
		}

		if (!open_ || !context_.view_.game_.cameraSystem_)
		{
			return;
		}

		ImGuiDockNode* dockNode = ImGui::DockBuilderGetNode(context_.graphics_.imgui_->DockSpaceID());
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 position = dockNode ? dockNode->Pos : viewport->WorkPos;
		ImVec2 size = dockNode ? dockNode->Size : viewport->WorkSize;
		ImGui::SetNextWindowPos(position);
		ImGui::SetNextWindowSize(size);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		if (ImGui::Begin("##GameFullscreen", nullptr, flags))
		{
			gameViewPanel_.Draw();
		}
		ImGui::End();
		ImGui::PopStyleVar(3);
	}

	void FullscreenPanel::Open()
	{
		open_ = true;
	}

	Bool FullscreenPanel::Opened()const
	{
		return open_;
	}
}
