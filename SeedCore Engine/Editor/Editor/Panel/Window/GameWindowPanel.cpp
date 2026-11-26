#include <Editor/Editor/Panel/Window/GameWindowPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <GraphicsEngine/System/CameraSystem.h>

namespace SeedCore
{
	GameWindowPanel::GameWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), gameViewPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void GameWindowPanel::Draw()
	{
		CameraSystem* cameraSystem = context_.view_.game_.cameraSystem_;
		if (!cameraSystem)
		{
			return;
		}

		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

		if (ImGui::Begin("ゲームビュー"))
		{
			Int currentMode = static_cast<Int>(cameraSystem->Mode());
			ImGui::RadioButton("Free", &currentMode, 0);
			ImGui::SameLine();
			ImGui::RadioButton("User", &currentMode, 1);
			cameraSystem->Mode(static_cast<CameraMode>(currentMode));

			ImVec2 regionSize = ImGui::GetContentRegionAvail();
			gameViewPanel_.Draw();

			if (regionSize.x > 0.0f && regionSize.y > 0.0f)
			{
				ImVec2 imageMin = ImGui::GetItemRectMin();
				ImVec2 imageMax = ImGui::GetItemRectMax();
				ImVec2 borderMin = ImVec2(imageMin.x - 1.0f, imageMin.y - 1.0f);
				ImVec2 borderMax = ImVec2(imageMax.x + 1.0f, imageMax.y + 1.0f);
				ImGui::GetWindowDrawList()->AddRect(borderMin, borderMax, ImGui::GetColorU32(ImGuiCol_Border));
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}
