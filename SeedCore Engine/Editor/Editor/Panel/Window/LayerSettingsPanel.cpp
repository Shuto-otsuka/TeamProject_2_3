#include <Editor/Editor/Panel/Window/LayerSettingsPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>

namespace SeedCore
{
	LayerSettingsPanel::LayerSettingsPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void LayerSettingsPanel::Open()
	{
		open_ = true;
	}

	void LayerSettingsPanel::DrawCollisionMatrixTab()
	{
		ImGui::TextDisabled("2つのレイヤーが物理的に衝突してよいかを設定します");
		ImGui::Spacing();

		DynamicArray<Size> usedLayers;
		for (Size index = 0; index < LayerRegistry::LayerCount; ++index)
		{
			if (!LayerRegistry::Name(index).view().empty())
			{
				usedLayers.push_back(index);
			}
		}

		if (usedLayers.empty())
		{
			ImGui::TextDisabled("(名前付きレイヤーがありません)");
			return;
		}

		std::span<const String> names = LayerRegistry::Names();

		constexpr Float headerRotation = 1.5707963f;

		Float maxNameWidth = 0.0f;
		for (Size layerIndex : usedLayers)
		{
			maxNameWidth = Max(maxNameWidth, ImGui::CalcTextSize(names[layerIndex].c_str()).x);
		}
		Float headerRowHeight = maxNameWidth + 40.0f;
		constexpr Float rowLabelWidth = 110.0f;

		Float checkboxColumnWidth = ImGui::GetFrameHeight() + 6.0f;
		Float headerTextColumnWidth = ImGui::GetTextLineHeight() + 18.0f;
		Float columnWidth = Max(checkboxColumnWidth, headerTextColumnWidth);

		Float tableHeight = Max(ImGui::GetContentRegionAvail().y - 60.0f, 200.0f);

		ImVec2 windowClipMin = ImGui::GetWindowPos();
		ImVec2 windowClipMax = ImVec2(windowClipMin.x + ImGui::GetWindowSize().x, windowClipMin.y + ImGui::GetWindowSize().y);

		Float contentWidth = rowLabelWidth + columnWidth * static_cast<Float>(usedLayers.size());
		Float availableWidth = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.y;
		Float tableWidth = Min(contentWidth, availableWidth);

		Int columnCount = static_cast<Int>(usedLayers.size()) + 1;
		if (ImGui::BeginTable("##LayerCollisionMatrix", columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, ImVec2(tableWidth, tableHeight)))
		{
			ImGui::TableSetupColumn("##RowLabel", ImGuiTableColumnFlags_WidthFixed, rowLabelWidth);
			for (Size layerIndex : usedLayers)
			{
				(void)layerIndex;
				ImGui::TableSetupColumn("##Col", ImGuiTableColumnFlags_WidthFixed, columnWidth);
			}

			ImGui::TableNextRow(ImGuiTableRowFlags_None, headerRowHeight);
			ImGui::TableSetColumnIndex(0);

			Float cosAngle = std::cos(headerRotation);
			Float sinAngle = std::sin(headerRotation);

			for (Size displayCol = 0; displayCol < usedLayers.size(); ++displayCol)
			{
				Size columnPos = usedLayers.size() - 1 - displayCol;

				ImGui::TableSetColumnIndex(static_cast<Int>(displayCol) + 1);

				std::string name = names[usedLayers[columnPos]].str();
				ImVec2 textSize = ImGui::CalcTextSize(name.c_str());

				ImVec2 cellMin = ImGui::GetCursorScreenPos();
				Float cellWidth = ImGui::GetContentRegionAvail().x;

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImVec2 nativeClipMin = drawList->GetClipRectMin();
				ImVec2 nativeClipMax = drawList->GetClipRectMax();
				Bool columnFullyVisible = (nativeClipMax.x - nativeClipMin.x) >= cellWidth - 1.0f;

				if (!columnFullyVisible)
				{
					continue;
				}

				ImVec2 pivot(cellMin.x + cellWidth * 0.5f + textSize.y * 0.5f, cellMin.y + 6.0f);
				ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

				drawList->PushClipRect(windowClipMin, windowClipMax, false);
				Int vertexStart = drawList->VtxBuffer.Size;
				drawList->AddText(pivot, textColor, name.c_str());
				Int vertexEnd = drawList->VtxBuffer.Size;
				drawList->PopClipRect();

				for (Int vertexIndex = vertexStart; vertexIndex < vertexEnd; ++vertexIndex)
				{
					ImDrawVert& vertex = drawList->VtxBuffer[vertexIndex];
					ImVec2 offset(vertex.pos.x - pivot.x, vertex.pos.y - pivot.y);
					vertex.pos = ImVec2(pivot.x + offset.x * cosAngle - offset.y * sinAngle, pivot.y + offset.x * sinAngle + offset.y * cosAngle);
				}
			}

			for (Size rowPos = 0; rowPos < usedLayers.size(); ++rowPos)
			{
				Size layerA = usedLayers[rowPos];

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(names[layerA].c_str());

				for (Size displayCol = 0; displayCol < usedLayers.size(); ++displayCol)
				{
					Size columnPos = usedLayers.size() - 1 - displayCol;

					ImGui::TableSetColumnIndex(static_cast<Int>(displayCol) + 1);

					if (columnPos < rowPos)
					{
						continue;
					}

					Size layerB = usedLayers[columnPos];

					Float checkboxOffsetX = (ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight()) * 0.5f;
					if (checkboxOffsetX > 0.0f)
					{
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + checkboxOffsetX);
					}

					ImGui::PushID(static_cast<Int>(rowPos * LayerRegistry::LayerCount + columnPos));
					Bool collide = LayerCollisionMatrix::GetCollide(layerA, layerB);
					if (ImGui::Checkbox("##Cell", &collide))
					{
						LayerCollisionMatrix::SetCollide(layerA, layerB, collide);
						LayerRegistry::Save();
					}
					ImGui::PopID();
				}
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();

		if (ImGui::Button("すべて無効"))
		{
			for (Size rowPos = 0; rowPos < usedLayers.size(); ++rowPos)
			{
				for (Size columnPos = rowPos; columnPos < usedLayers.size(); ++columnPos)
				{
					LayerCollisionMatrix::SetCollide(usedLayers[rowPos], usedLayers[columnPos], false);
				}
			}
			LayerRegistry::Save();
		}
		ImGui::SameLine();
		if (ImGui::Button("すべて有効"))
		{
			for (Size rowPos = 0; rowPos < usedLayers.size(); ++rowPos)
			{
				for (Size columnPos = rowPos; columnPos < usedLayers.size(); ++columnPos)
				{
					LayerCollisionMatrix::SetCollide(usedLayers[rowPos], usedLayers[columnPos], true);
				}
			}
			LayerRegistry::Save();
		}
	}

	void LayerSettingsPanel::Draw()
	{
		if (!open_)
		{
			return;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		ImGui::SetNextWindowSize(ImVec2(860, 800), ImGuiCond_Appearing);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDocking;

		if (ImGui::Begin("レイヤー設定", &open_, flags))
		{
			DrawCollisionMatrixTab();
		}
		ImGui::End();
	}
}
