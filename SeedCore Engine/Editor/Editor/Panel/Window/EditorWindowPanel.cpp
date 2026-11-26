#include <Editor/Editor/Panel/Window/EditorWindowPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>

namespace SeedCore
{
	EditorWindowPanel::EditorWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), viewModePanel_(context, imguiTexture), editorViewPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void EditorWindowPanel::Draw()
	{
		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

		if (!ImGui::FindWindowByName("エディタービュー"))
		{
			ImGui::SetNextWindowFocus();
		}

		if (ImGui::Begin("エディタービュー"))
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				context_.view_.focused_ = FocusedView::Editor;
			}

			DrawGuizmo();
			ImGui::Separator();

			editorViewPanel_.Draw();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorWindowPanel::DrawGuizmo()
	{
		ImVec2 iconSize(24, 24);
		ImVec4 transparent(0, 0, 0, 0);
		ImVec4 activeColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		ImVec4 hoverColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);

		auto& op = context_.guizmo_.operation_;

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

		ImGui::PushStyleColor(ImGuiCol_Button, !context_.guizmo_.visible_ ? activeColor : transparent);
		if (ImGui::ImageButton("##NonSelected", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/NonSelected"))), iconSize))
		{
			context_.guizmo_.visible_ = !context_.guizmo_.visible_;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = (ImGuizmo::OPERATION)0;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::TRANSLATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Translate", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Translate"))), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::TRANSLATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::ROTATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Rotate", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Rotate"))), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::ROTATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::SCALE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Scale", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Scale"))), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::SCALE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.guizmo_.pivotEdit_ ? activeColor : transparent);
		if (ImGui::ImageButton("##Pivot", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Pivot"))), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("基準点を編集");
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.guizmo_.rectTool_ ? activeColor : transparent);
		if (ImGui::ImageButton("##Rect", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Rect"))), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = true;
			context_.guizmo_.pivotEdit_ = false;
		}
		ImGui::PopStyleColor();

		ImGui::PopStyleColor(2);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##GizmoIcon", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Guizmo"))), iconSize))
		{
			ImGui::OpenPopup("##GizmoSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##GizmoSettings"))
		{
			ImGui::SeparatorText("スナップ");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("移動", &context_.guizmo_.translateSnap_, 0.1f, 0.01f, 100.0f, "%.2f");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("回転", &context_.guizmo_.rotateSnap_, 0.5f, 0.1f, 90.0f, "%.0f\xc2\xb0");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("拡大縮小", &context_.guizmo_.scaleSnap_, 0.05f, 0.01f, 10.0f, "%.2f");

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.view_.editor_.iconVisible_ ? activeColor : transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ShowIcon", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/ShowIcon"))), iconSize))
		{
			context_.view_.editor_.iconVisible_ = !context_.view_.editor_.iconVisible_;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("アイコンを表示");
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.view_.editor_.shapeVisible_ ? activeColor : transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ShowShape", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/ShowShape"))), iconSize))
		{
			context_.view_.editor_.shapeVisible_ = !context_.view_.editor_.shapeVisible_;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("範囲などの形を常に表示");
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##CameraIcon", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/Camera"))), iconSize))
		{
			ImGui::OpenPopup("##CameraSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##CameraSettings"))
		{
			if (context_.view_.editor_.camera_)
			{
				EditorCamera& camera = *context_.view_.editor_.camera_;
				ImGui::SeparatorText("投影");

				Float nearPlane = camera.Near();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Near", &nearPlane, 0.001f, 0.0001f, 10.0f, "%.4f"))
				{
					camera.Near(nearPlane);
				}

				Float farPlane = camera.Far();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Far", &farPlane, 1.0f, 1.0f, 100000.0f, "%.0f"))
				{
					camera.Far(farPlane);
				}

				Float fov = camera.Fov();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("FOV", &fov, 0.5f, 1.0f, 179.0f, "%.1f"))
				{
					camera.Fov(fov);
				}
			}

			if (context_.view_.editor_.cameraController_)
			{
				EditorCameraController& controller = *context_.view_.editor_.cameraController_;
				ImGui::SeparatorText("操作速度");

				Float moveSpeed = controller.MoveSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("移動", &moveSpeed, 0.1f, 0.1f, 200.0f, "%.1f"))
				{
					controller.MoveSpeed(moveSpeed);
				}

				Float rotateSpeed = controller.RotateSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("回転", &rotateSpeed, 0.01f, 0.01f, 2.0f, "%.2f"))
				{
					controller.RotateSpeed(rotateSpeed);
				}

				Float scrollSpeed = controller.ScrollSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("ズーム", &scrollSpeed, 0.1f, 0.1f, 100.0f, "%.1f"))
				{
					controller.ScrollSpeed(scrollSpeed);
				}

				Float panSpeed = controller.PanSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("パン", &panSpeed, 0.001f, 0.001f, 1.0f, "%.3f"))
				{
					controller.PanSpeed(panSpeed);
				}

				Float shiftMultiplier = controller.ShiftSpeedMultiplier();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Shift倍率", &shiftMultiplier, 0.1f, 1.0f, 20.0f, "x%.1f"))
				{
					controller.ShiftSpeedMultiplier(shiftMultiplier);
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ViewModeIcon", static_cast<ImTextureID>(imguiTexture_.Icon(String("Viewport/ViewMode"))), iconSize))
		{
			ImGui::OpenPopup("##ViewModeSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##ViewModeSettings"))
		{
			viewModePanel_.Draw();
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);
	}
}
