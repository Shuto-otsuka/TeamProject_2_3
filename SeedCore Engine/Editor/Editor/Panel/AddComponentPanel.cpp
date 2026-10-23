#include <Editor/Editor/Panel/AddComponentPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Command/ComponentLifecycleCommand.h>

namespace SeedCore
{
	AddComponentPanel::AddComponentPanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

	void AddComponentPanel::Draw(Actor actor, ImGuiTexture& imguiTexture)
	{
		Float availableWidth = ImGui::GetContentRegionAvail().x;
		Float buttonWidth = 200.0f;
		ImGui::SetCursorPosX((availableWidth - buttonWidth) * 0.5f);

		if (ImGui::Button("コンポーネントを追加", ImVec2(buttonWidth, 0)))
		{
			state_.searchBuffer = String("");
			ImGui::OpenPopup("AddComponentPopup");
		}

		ImVec2 buttonMin = ImGui::GetItemRectMin();
		ImVec2 buttonMax = ImGui::GetItemRectMax();
		Float buttonCenterX = (buttonMin.x + buttonMax.x) * 0.5f;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		Float spaceBelow = (viewport->WorkPos.y + viewport->WorkSize.y) - buttonMax.y;
		Float spaceAbove = buttonMin.y - viewport->WorkPos.y;

		constexpr Float minReasonableSpace = 150.0f;
		constexpr Float viewportEdgeMargin = 16.0f;
		Bool opensDownward = spaceBelow >= minReasonableSpace || spaceBelow >= spaceAbove;
		Float maxPopupHeight = (opensDownward ? spaceBelow : spaceAbove) - viewportEdgeMargin;
		ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxPopupHeight));

		if (opensDownward)
		{
			ImGui::SetNextWindowPos(ImVec2(buttonCenterX, buttonMax.y), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
		}
		else
		{
			ImGui::SetNextWindowPos(ImVec2(buttonCenterX, buttonMin.y), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
		}

		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			ImGui::SetNextItemWidth(240.0f);
			DrawSearchBar(imguiTexture);
			ImGui::Separator();
			DrawComponentList(actor, imguiTexture);
			ImGui::EndPopup();
		}
	}

	void AddComponentPanel::DrawSearchBar(ImGuiTexture& imguiTexture)
	{
		std::string searchText = state_.searchBuffer.str();
		searchText.resize(256);

		Float iconSize = ImGui::GetTextLineHeight();
		Float originalPaddingX = ImGui::GetStyle().FramePadding.x;
		Float iconPadding = iconSize + originalPaddingX * 2.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(iconPadding, ImGui::GetStyle().FramePadding.y));

		if (ImGui::InputTextWithHint("##Search", "検索...", searchText.data(), searchText.capacity()))
		{
			state_.searchBuffer = String(std::string_view(searchText.c_str()));
		}

		ImGui::PopStyleVar();
		ImVec2 inputMin = ImGui::GetItemRectMin();
		Float inputHeight = ImGui::GetItemRectSize().y;
		Float iconY = inputMin.y + (inputHeight - iconSize) * 0.5f;
		ImGui::GetWindowDrawList()->AddImage(imguiTexture.Icon(IconType::Search), ImVec2(inputMin.x + originalPaddingX, iconY), ImVec2(inputMin.x + originalPaddingX + iconSize, iconY + iconSize));
	}

	void AddComponentPanel::DrawComponentList(Actor actor, ImGuiTexture& imguiTexture)
	{
		std::string filterText = state_.searchBuffer.str();

		/// [EN] Group by ComponentMetadata::category_ (e.g. "Light"). std::map
		///      keeps categories alphabetically sorted for free.
		/// [JP] ComponentMetadata::category_（例: "Light"）でグループ化。
		///      std::map によってカテゴリはそのままアルファベット順になる。
		std::map<std::string, DynamicArray<std::pair<String, ComponentID>>> grouped;

		for (ComponentID componentID : ComponentRegistry::Registry())
		{
			const String& componentName = componentID->name_;
			if (CheckBuiltinComponent(componentName))
			{
				continue;
			}
			if (!CheckFilterMatch(componentName, filterText))
			{
				continue;
			}

			std::string category = componentID->category_.str();
			grouped[category].emplace_back(componentName, componentID);
		}

		if (grouped.empty())
		{
			ImGui::TextDisabled("該当なし");
			return;
		}

		Bool searching = !filterText.empty();

		static const std::string customCategory = "Custom";

		auto drawCategory = [&](const std::string& category, DynamicArray<std::pair<String, ComponentID>>& entries)
		{
			std::ranges::sort(entries, [](const auto& a, const auto& b)
				{
					return a.first.str() < b.first.str();
				});

			if (searching)
			{
				for (auto& [componentName, componentID] : entries)
				{
					DrawMenuItem(actor, componentName, componentID, imguiTexture);
				}
			}
			else if (ImGui::BeginMenu(category.c_str()))
			{
				for (auto& [componentName, componentID] : entries)
				{
					DrawMenuItem(actor, componentName, componentID, imguiTexture);
				}
				ImGui::EndMenu();
			}
		};

		for (auto& [category, entries] : grouped)
		{
			if (category == customCategory)
			{
				continue;
			}

			drawCategory(category, entries);
		}

		auto customEntries = grouped.find(customCategory);
		if (customEntries != grouped.end())
		{
			ImGui::Separator();
			drawCategory(customEntries->first, customEntries->second);
		}
	}

	void AddComponentPanel::DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID, ImGuiTexture& imguiTexture)
	{
		Bool alreadyAttached = actor.HasComponent(componentID);

		if (alreadyAttached)
		{
			ImGui::BeginDisabled();
		}

		Float iconSize = ImGui::GetTextLineHeight();
		ImGui::Image(imguiTexture.Icon(ImGuiTexture::ComponentIconType(componentName)), ImVec2(iconSize, iconSize));
		ImGui::SameLine();

		if (ImGui::Selectable(componentName.c_str(), false, ImGuiSelectableFlags_NoAutoClosePopups))
		{
			context_.scene_.history_.Push(MakePtr<ComponentAddCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, componentID));
			actor.AddComponent(componentID);
			ImGui::CloseCurrentPopup();
		}

		if (alreadyAttached)
		{
			ImGui::EndDisabled();
		}
	}

	Bool AddComponentPanel::CheckBuiltinComponent(const String& componentName)const
	{
		static const String builtinComponents[] =
		{
			String("Name"),
			String("Position"),
			String("Rotation"),
			String("Scale"),
			String("WorldMatrix"),
			String("LinearVelocity"),
			String("AngularVelocity"),
			String("Active"),
			String("Bounds"),
			String("Material"),
			String("Skeleton"),

			/// [EN] Only ever made by loading a component whose script is missing, never added by hand.
			/// [JP] スクリプトの無いコンポーネントを読み込んだときにだけ作られるもので、手で追加するものではない。
			String("UnknownComponent")
		};

		return std::ranges::contains(builtinComponents, componentName);
	}

	Bool AddComponentPanel::CheckFilterMatch(const String& componentName, const std::string& filterText)const
	{
		if (filterText.empty())
		{
			return true;
		}

		std::string targetName = componentName.str();
		auto found = std::ranges::search(targetName, filterText, [](char a, char b) { return std::tolower(a) == std::tolower(b); });
		return !found.empty();
	}
}
