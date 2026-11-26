#include <Editor/Editor/Panel/Embed/ConsolePanel.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/Log/LogSystem.h>

namespace SeedCore
{
	ConsolePanel::ConsolePanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void ConsolePanel::Draw()
	{
		const auto& logs = LogSystem::GetLogs();

		Uint32 errorCount = 0;
		Uint32 warningCount = 0;
		Uint32 noticeCount = 0;
		for (const LogEntry& entry : logs)
		{
			switch (entry.level_)
			{
			case LogLevel::Error:
				errorCount++;
				break;
			case LogLevel::Warning:
				warningCount++;
				break;
			case LogLevel::Notice:
				noticeCount++;
				break;
			}
		}

		if (ImGui::Button("クリア"))
		{
			LogSystem::Clear();
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		ImVec2 iconSize(14, 14);

		auto FilterButton = [&](const Char* id, const String& icon, Bool& flag, Uint32 count, const ImVec4& color)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, flag ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) : ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));

			Char label[64];
			snprintf(label, sizeof(label), "%u", count);
			Float textWidth = ImGui::CalcTextSize(label).x;
			Float totalWidth = iconSize.x + 4.0f + textWidth + ImGui::GetStyle().FramePadding.x * 2.0f;
			Float buttonHeight = ImGui::GetFrameHeight();

			ImVec2 cursor = ImGui::GetCursorScreenPos();
			if (ImGui::InvisibleButton(id, ImVec2(totalWidth, buttonHeight)))
			{
				flag = !flag;
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			if (ImGui::IsItemHovered())
			{
				drawList->AddRectFilled(cursor, ImVec2(cursor.x + totalWidth, cursor.y + buttonHeight), ImGui::GetColorU32(ImGuiCol_HeaderHovered), ImGui::GetStyle().FrameRounding);
			}
			else if (flag)
			{
				drawList->AddRectFilled(cursor, ImVec2(cursor.x + totalWidth, cursor.y + buttonHeight), ImGui::GetColorU32(ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding);
			}

			Float iconY = cursor.y + (buttonHeight - iconSize.y) * 0.5f;
			Float iconX = cursor.x + ImGui::GetStyle().FramePadding.x;
			drawList->AddImage(imguiTexture_.Icon(icon), ImVec2(iconX, iconY), ImVec2(iconX + iconSize.x, iconY + iconSize.y));

			Float textX = iconX + iconSize.x + 4.0f;
			Float textY = cursor.y + (buttonHeight - ImGui::GetTextLineHeight()) * 0.5f;
			drawList->AddText(ImVec2(textX, textY), ImGui::GetColorU32(color), label);

			ImGui::PopStyleColor(2);
		};

		FilterButton("##WarningFilter", String("Log/Warning"), showWarning_, warningCount, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
		ImGui::SameLine();
		FilterButton("##ErrorFilter",   String("Log/Error"),   showError_,   errorCount,   ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
		ImGui::SameLine();
		FilterButton("##NoticeFilter",  String("Log/Notice"),  showNotice_,  noticeCount,  ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

		ImGui::Separator();

		ImGui::BeginChild("##LogArea", ImVec2(0, 0), false, ImGuiWindowFlags_None);

		Float rowWidth = ImGui::GetContentRegionAvail().x;

		ImGuiListClipper clipper;
		DynamicArray<Uint32> visibleIndices;
		for (Uint32 index = 0; index < static_cast<Uint32>(logs.size()); index++)
		{
			if (logs[index].level_ == LogLevel::Notice && !showNotice_)
			{
				continue;
			}
			if (logs[index].level_ == LogLevel::Warning && !showWarning_)
			{
				continue;
			}
			if (logs[index].level_ == LogLevel::Error && !showError_)
			{
				continue;
			}
			visibleIndices.push_back(index);
		}

		for (Uint32 row = 0; row < static_cast<Uint32>(visibleIndices.size()); row++)
		{
			const auto& entry = logs[visibleIndices[row]];

			std::string rowText = std::format("{}  ({}:{})", entry.message_.str(), entry.file_.str(), entry.line_);

			Float textOffsetX = 4.0f + 14.0f + 6.0f;
			ImVec2 textSize = ImGui::CalcTextSize(rowText.c_str(), rowText.c_str() + rowText.size(), false, rowWidth - textOffsetX);
			Float rowHeight = (textSize.y > 14.0f ? textSize.y : 14.0f) + 6.0f;

			ImVec4 color;
			String icon;
			switch (entry.level_)
			{
			case LogLevel::Error:
				color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				icon = String("Log/Error");
				break;
			case LogLevel::Warning:
				color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
				icon = String("Log/Warning");
				break;
			default:
				color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
				icon = String("Log/Notice");
				break;
			}

			ImVec2 position = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			if (row % 2 == 1)
			{
				drawList->AddRectFilled(position, ImVec2(position.x + rowWidth, position.y + rowHeight), IM_COL32(255, 255, 255, 10));
			}

			ImVec2 logIconSize(14, 14);
			Float iconY = position.y + (rowHeight - logIconSize.y) * 0.5f;
			drawList->AddImage(imguiTexture_.Icon(icon), ImVec2(position.x + 4.0f, iconY), ImVec2(position.x + 4.0f + logIconSize.x, iconY + logIconSize.y));

			ImGui::SetCursorScreenPos(ImVec2(position.x + textOffsetX, position.y + 3.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + rowWidth - textOffsetX);
			ImGui::TextUnformatted(rowText.c_str(), rowText.c_str() + rowText.size());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();

			ImGui::SetCursorScreenPos(position);
			Char rowId[32];
			snprintf(rowId, sizeof(rowId), "##logRow%u", visibleIndices[row]);
			ImGui::InvisibleButton(rowId, ImVec2(rowWidth, rowHeight));
			if (ImGui::BeginPopupContextItem(rowId))
			{
				if (ImGui::MenuItem("この行をコピー"))
				{
					ImGui::SetClipboardText(rowText.c_str());
				}
				if (ImGui::MenuItem("メッセージのみコピー"))
				{
					ImGui::SetClipboardText(entry.message_.c_str());
				}
				ImGui::EndPopup();
			}

			ImGui::SetCursorScreenPos(ImVec2(position.x, position.y + rowHeight));
			ImGui::Dummy(ImVec2(0, 0));
		}

		if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
	}
}
