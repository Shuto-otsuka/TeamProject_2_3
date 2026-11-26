#include <Editor/Editor/Panel/Embed/AddComponentPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Command/ComponentLifecycleCommand.h>

namespace SeedCore
{
	AddComponentPanel::AddComponentPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void AddComponentPanel::Draw(Actor actor)
	{
		Float availableWidth = ImGui::GetContentRegionAvail().x;
		Float buttonWidth = 200.0f;
		ImGui::SetCursorPosX((availableWidth - buttonWidth) * 0.5f);

		if (ImGui::Button("コンポーネントを追加", ImVec2(buttonWidth, 0)))
		{
			search_ = String("");
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
			DrawSearchBar();
			ImGui::Separator();
			DrawComponent(actor);
			ImGui::EndPopup();
		}
	}

	void AddComponentPanel::DrawSearchBar()
	{
		std::string searchText = search_.str();
		searchText.resize(256);

		Float iconSize = ImGui::GetTextLineHeight();
		Float originalPaddingX = ImGui::GetStyle().FramePadding.x;
		Float iconPadding = iconSize + originalPaddingX * 2.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(iconPadding, ImGui::GetStyle().FramePadding.y));

		if (ImGui::InputTextWithHint("##Search", "検索...", searchText.data(), searchText.capacity()))
		{
			search_ = String(std::string_view(searchText.c_str()));
		}

		ImGui::PopStyleVar();
		ImVec2 inputMin = ImGui::GetItemRectMin();
		Float inputHeight = ImGui::GetItemRectSize().y;
		Float iconY = inputMin.y + (inputHeight - iconSize) * 0.5f;
		ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Search"))), ImVec2(inputMin.x + originalPaddingX, iconY), ImVec2(inputMin.x + originalPaddingX + iconSize, iconY + iconSize));
	}

	void AddComponentPanel::DrawComponent(Actor actor)
	{
		std::string filterText = search_.str();
		Bool searching = !filterText.empty();

		matches_.clear();
		for (ComponentID componentID : ComponentRegistry::Registry())
		{
			const String& componentName = componentID->name_;
			if (CheckBuiltin(componentName) || !CheckFilter(componentName, filterText))
			{
				continue;
			}
			matches_.emplace_back(componentName, componentID);
		}

		if (matches_.empty())
		{
			ImGui::TextDisabled("該当なし");
			return;
		}

		std::ranges::sort(matches_, [searching](const auto& a, const auto& b)
			{
				if (!searching)
				{
					std::string_view categoryA = static_cast<std::string_view>(a.second->category_);
					std::string_view categoryB = static_cast<std::string_view>(b.second->category_);
					if (categoryA != categoryB)
					{
						return categoryA < categoryB;
					}
				}
				return static_cast<std::string_view>(a.first) < static_cast<std::string_view>(b.first);
			});

		if (searching)
		{
			for (auto& [componentName, componentID] : matches_)
			{
				DrawMenuItem(actor, componentName, componentID);
			}
			return;
		}

		static const std::string customCategory = "Custom";

		DynamicArray<std::string_view> roots;
		for (auto& [componentName, componentID] : matches_)
		{
			std::string_view category = static_cast<std::string_view>(componentID->category_);
			std::string_view root = category.substr(0, category.find('/'));
			if (!std::ranges::contains(roots, root))
			{
				roots.push_back(root);
			}
		}

		for (std::string_view root : roots)
		{
			if (root == customCategory)
			{
				continue;
			}

			std::string rootPath = std::string(root);
			if (ImGui::BeginMenu(rootPath.c_str()))
			{
				DrawCategory(actor, rootPath);
				ImGui::EndMenu();
			}
		}

		if (std::ranges::contains(roots, std::string_view(customCategory)))
		{
			ImGui::Separator();
			if (ImGui::BeginMenu(customCategory.c_str()))
			{
				DrawCategory(actor, customCategory);
				ImGui::EndMenu();
			}
		}
	}

	void AddComponentPanel::DrawCategory(Actor actor, const std::string& path)
	{
		auto first = std::ranges::lower_bound(matches_, std::string_view(path), {}, [](const auto& match) { return static_cast<std::string_view>(match.second->category_); });
		auto last = std::find_if(first, matches_.end(), [&](const auto& match) { return !static_cast<std::string_view>(match.second->category_).starts_with(path); });

		DynamicArray<std::string_view> children;
		for (auto it = first; it != last; ++it)
		{
			std::string_view category = static_cast<std::string_view>(it->second->category_);
			if (category.size() <= path.size() || category[path.size()] != '/')
			{
				continue;
			}
			std::string_view rest = category.substr(path.size() + 1);
			std::string_view child = rest.substr(0, rest.find('/'));
			if (!std::ranges::contains(children, child))
			{
				children.push_back(child);
			}
		}

		for (std::string_view child : children)
		{
			std::string childName = std::string(child);
			if (ImGui::BeginMenu(childName.c_str()))
			{
				DrawCategory(actor, path + "/" + childName);
				ImGui::EndMenu();
			}
		}

		for (auto it = first; it != last; ++it)
		{
			if (static_cast<std::string_view>(it->second->category_) == path)
			{
				DrawMenuItem(actor, it->first, it->second);
			}
		}
	}

	void AddComponentPanel::DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID)
	{
		Bool alreadyAttached = actor.HasComponent(componentID);

		if (alreadyAttached)
		{
			ImGui::BeginDisabled();
		}

		Float iconSize = ImGui::GetTextLineHeight();
		ImGui::Image(static_cast<ImTextureID>(imguiTexture_.Icon(String("Component/" + componentName.str()), String("Component/Custom"))), ImVec2(iconSize, iconSize));
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

	Bool AddComponentPanel::CheckBuiltin(const String& componentName)const
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
			String("Selected"),

			String("UnknownComponent")
		};

		return std::ranges::contains(builtinComponents, componentName);
	}

	Bool AddComponentPanel::CheckFilter(const String& componentName, const std::string& filterText)const
	{
		if (filterText.empty())
		{
			return true;
		}

		std::string_view targetName = static_cast<std::string_view>(componentName);
		auto found = std::ranges::search(targetName, filterText, [](Char a, Char b) { return std::tolower(static_cast<Uchar>(a)) == std::tolower(static_cast<Uchar>(b)); });
		return !found.empty();
	}
}
