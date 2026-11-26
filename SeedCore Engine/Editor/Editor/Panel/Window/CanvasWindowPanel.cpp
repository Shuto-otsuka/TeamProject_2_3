#include <Editor/Editor/Panel/Window/CanvasWindowPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>

namespace SeedCore
{
	CanvasWindowPanel::CanvasWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), canvasViewPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void CanvasWindowPanel::Draw()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

		if (ImGui::Begin("キャンバスビュー"))
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				context_.view_.focused_ = FocusedView::Canvas;
			}

			DrawGizmoMenu();
			ImGui::Separator();

			canvasViewPanel_.Draw();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void CanvasWindowPanel::DrawGizmoMenu()
	{
		ImVec2 iconSize(24, 24);
		ImVec4 transparent(0, 0, 0, 0);
		ImVec4 activeColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		ImVec4 hoverColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);

		auto& op = context_.guizmo_.operation_;

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

		ImGui::PushStyleColor(ImGuiCol_Button, !context_.guizmo_.visible_ ? activeColor : transparent);
		if (ImGui::ImageButton("##NonSelected", imguiTexture_.Icon(IconType::NonSelected), iconSize))
		{
			context_.guizmo_.visible_ = !context_.guizmo_.visible_;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = (ImGuizmo::OPERATION)0;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::TRANSLATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Translate", imguiTexture_.Icon(IconType::Translate), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::TRANSLATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::ROTATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Rotate", imguiTexture_.Icon(IconType::Rotate), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::ROTATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && !context_.guizmo_.pivotEdit_ && op == ImGuizmo::SCALE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Scale", imguiTexture_.Icon(IconType::Scale), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			context_.guizmo_.pivotEdit_ = false;
			op = ImGuizmo::SCALE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.guizmo_.pivotEdit_ ? activeColor : transparent);
		if (ImGui::ImageButton("##Pivot", imguiTexture_.Icon(IconType::Pivot), iconSize))
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
		if (ImGui::ImageButton("##Rect", imguiTexture_.Icon(IconType::Rect), iconSize))
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
		if (ImGui::ImageButton("##GizmoIcon2D", imguiTexture_.Icon(IconType::Guizmo), iconSize))
		{
			ImGui::OpenPopup("##GizmoSettings2D");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##GizmoSettings2D"))
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
	}
}
