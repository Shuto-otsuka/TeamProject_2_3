#include <Editor/Editor/Panel/Window/EffectGraphPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/Effect/EffectResource.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrNodeRegistry.h>

namespace SeedCore
{
	EffectGraphPanel::EffectGraphPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void EffectGraphPanel::Draw()
	{
		if (!open_)
		{
			return;
		}

		EffectResource& resource = *context_.graphics_.graphics_->Module().Resources().effectResource_;
		AssetRecord* record = context_.world_.resource_->Record(assetID_);
		EffectAsset* effect = record ? resource.Resolve(resource.Load(*context_.world_.resource_, assetID_)) : nullptr;

		std::string title = std::format("エフェクトグラフ {}###EffectGraph", dirty_ ? " *" : "");
		if (ImGui::Begin(title.c_str(), &open_))
		{
			if (!effect)
			{
				ImGui::TextDisabled("エフェクトが開かれていません");
			}
			else
			{
				if (ImGui::Button("保存"))
				{
					if (resource.Bake(AssetContext{ *context_.world_.resource_, LoadThread::Main }, assetID_, record->fullpath_))
					{
						dirty_ = false;
					}
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%s", record->path_.c_str());

				if (ImGui::BeginTabBar("##EffectTabs"))
				{
					if (ImGui::BeginTabItem("グラフ"))
					{
						DrawGraph(*effect);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("ビュー"))
					{
						DrawView(*effect);
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			}
		}
		ImGui::End();
	}

	void EffectGraphPanel::Open(Uint32 assetID)
	{
		ImGuiWindow* editorViewWindow = ImGui::FindWindowByName("エディタービュー");
		ImGuiID dockID = (editorViewWindow && editorViewWindow->DockId != 0) ? editorViewWindow->DockId : context_.graphics_.imgui_->DockSpaceID();
		if (!open_)
		{
			ImGui::DockBuilderDockWindow("###EffectGraph", dockID);
		}

		open_ = true;
		ImGui::SetWindowFocus("###EffectGraph");

		if (assetID != assetID_)
		{
			assetID_ = assetID;
			selectedEmitterIndex_ = 0;
			dirty_ = false;
			canvas_.Deselect();
		}
	}

	void EffectGraphPanel::Inspect()
	{
		EffectResource& resource = *context_.graphics_.graphics_->Module().Resources().effectResource_;
		EffectAsset* effect = resource.Resolve(resource.GetHandle(assetID_));
		if (!effect || selectedEmitterIndex_ >= effect->emitters_.size())
		{
			ImGui::TextDisabled("エフェクトが開かれていません");
			return;
		}

		EffectEmitter& emitter = effect->emitters_[selectedEmitterIndex_];
		DynamicArray<Uint32> selected = canvas_.SelectedNodes();
		if (selected.empty())
		{
			Char name[256] = {};
			strncpy_s(name, emitter.name_.c_str(), _TRUNCATE);
			if (ImGui::InputText("名前", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				emitter.name_ = String(name);
				dirty_ = true;
			}
			if (ImGui::Checkbox("有効", &emitter.enabled_))
			{
				dirty_ = true;
			}
			return;
		}

		auto node = std::ranges::find_if(emitter.nodes_, [&selected](const EffectNode& candidate) { return candidate.id_ == selected.front(); });
		if (node == emitter.nodes_.end())
		{
			return;
		}
		const ZephyrNodeDefinition* definition = ZephyrNodeRegistry::Find(node->type_);
		ImGui::TextUnformatted(definition ? definition->label_.c_str() : node->type_.c_str());
		ImGui::Separator();
		if (!definition)
		{
			return;
		}

		for (const ZephyrNodeInput& input : definition->inputs_)
		{
			Bool connected = std::ranges::any_of(emitter.links_, [&node, &input](const EffectLink& link) { return link.toNode_ == node->id_ && link.toPin_ == input.name_; });
			if (connected)
			{
				ImGui::TextDisabled("%s 接続中", input.name_.c_str());
				continue;
			}

			auto found = std::ranges::find_if(node->parameters_, [&input](const EffectParameter& parameter) { return parameter.name_ == input.name_; });
			EffectParameter parameter = found != node->parameters_.end() ? *found : EffectParameter{ input.name_, input.default_, 0 };
			Bool changed = false;

			ImGui::PushID(input.name_.c_str());
			switch (input.type_)
			{
			case ZephyrType::Float:
				changed = ImGui::DragFloat(input.name_.c_str(), &parameter.value_.x, 0.01f);
				break;
			case ZephyrType::Float2:
				changed = ImGui::DragFloat2(input.name_.c_str(), &parameter.value_.x, 0.01f);
				break;
			case ZephyrType::Float3:
				changed = ImGui::DragFloat3(input.name_.c_str(), &parameter.value_.x, 0.01f);
				break;
			case ZephyrType::Float4:
				changed = ImGui::DragFloat4(input.name_.c_str(), &parameter.value_.x, 0.01f);
				break;
			case ZephyrType::Color:
				changed = ImGui::ColorEdit4(input.name_.c_str(), &parameter.value_.x, ImGuiColorEditFlags_Float);
				break;
			case ZephyrType::Int:
			{
				Int value = static_cast<Int>(parameter.value_.x);
				if (ImGui::DragInt(input.name_.c_str(), &value))
				{
					parameter.value_.x = static_cast<Float>(value);
					changed = true;
				}
				break;
			}
			case ZephyrType::Bool:
			{
				Bool value = parameter.value_.x != 0.0f;
				if (ImGui::Checkbox(input.name_.c_str(), &value))
				{
					parameter.value_.x = value ? 1.0f : 0.0f;
					changed = true;
				}
				break;
			}
			case ZephyrType::Effekseer:
			{
				AssetRecord* record = parameter.asset_ != 0 ? context_.world_.resource_->Record(parameter.asset_) : nullptr;
				ImGui::Button(record ? record->path_.c_str() : "（なし）", ImVec2(ImGui::CalcItemWidth(), 0.0f));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_EFFEKSEER"))
					{
						parameter.asset_ = *static_cast<const Uint32*>(payload->Data);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SameLine();
				Float iconHeight = ImGui::GetTextLineHeight();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				if (ImGui::ImageButton("##Clear", imguiTexture_.Icon(String("Misc/Cross")), ImVec2(iconHeight, iconHeight)))
				{
					parameter.asset_ = 0;
					changed = true;
				}
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::TextUnformatted(input.name_.c_str());
				break;
			}
			default:
				ImGui::TextDisabled("%s", input.name_.c_str());
				break;
			}
			ImGui::PopID();

			if (changed)
			{
				if (found != node->parameters_.end())
				{
					*found = parameter;
				}
				else
				{
					node->parameters_.push_back(parameter);
				}
				dirty_ = true;
			}
		}
	}

	Bool EffectGraphPanel::Focused()const
	{
		ImGuiWindow* graph = ImGui::FindWindowByName("###EffectGraph");
		return open_ && graph && !graph->SkipItems;
	}

	void EffectGraphPanel::DrawGraph(EffectAsset& effect)
	{
		DrawEmitters(effect);
		ImGui::SameLine();

		if (selectedEmitterIndex_ >= effect.emitters_.size())
		{
			selectedEmitterIndex_ = 0;
		}

		if (effect.emitters_.empty())
		{
			ImGui::TextDisabled("エミッターがありません");
			return;
		}

		EffectEmitter& emitter = effect.emitters_[selectedEmitterIndex_];
		ImGui::BeginGroup();
		ImGui::TextUnformatted(emitter.name_.c_str());
		ImGui::Separator();
		canvas_.Begin("EffectGraphCanvas");
		DrawNodes(emitter);
		DrawLinks(emitter);
		UpdateConnect(emitter);
		DrawMenu(emitter);
		canvas_.End();
		ImGui::EndGroup();
		UpdateLayout(emitter);
	}

	void EffectGraphPanel::DrawView(EffectAsset& effect)
	{
		ImGui::TextDisabled("プレビューはこれから");
	}

	void EffectGraphPanel::DrawEmitters(EffectAsset& effect)
	{
		ImGui::BeginChild("##Emitters", ImVec2(160.0f, 0.0f), ImGuiChildFlags_Borders);
		for (Size emitterIndex = 0;emitterIndex < effect.emitters_.size();++emitterIndex)
		{
			EffectEmitter& emitter = effect.emitters_[emitterIndex];
			ImGui::PushID(static_cast<Int>(emitterIndex));
			if (ImGui::Checkbox("##Enabled", &emitter.enabled_))
			{
				dirty_ = true;
			}
			ImGui::SameLine();
			if (ImGui::Selectable(emitter.name_.c_str(), selectedEmitterIndex_ == emitterIndex))
			{
				selectedEmitterIndex_ = emitterIndex;
				canvas_.Deselect();
			}
			ImGui::PopID();
		}
		if (ImGui::Button("+ 追加", ImVec2(-1.0f, 0.0f)))
		{
			effect.emitters_.push_back(EffectEmitter::Create(String(std::format("Emitter {}", effect.emitters_.size() + 1))));
			selectedEmitterIndex_ = effect.emitters_.size() - 1;
			canvas_.Deselect();
			dirty_ = true;
		}
		ImGui::EndChild();
	}

	void EffectGraphPanel::DrawNodes(EffectEmitter& emitter)
	{
		auto valueText = [](ZephyrType type, const Vector4& value)
			{
				switch (type)
				{
				case ZephyrType::Float:
					return std::format("{:.2f}", value.x);
				case ZephyrType::Float2:
					return std::format("{:.2f}, {:.2f}", value.x, value.y);
				case ZephyrType::Float3:
					return std::format("{:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z);
				case ZephyrType::Float4:
					[[fallthrough]];
				case ZephyrType::Color:
					return std::format("{:.2f}, {:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z, value.w);
				case ZephyrType::Int:
					return std::format("{}", static_cast<Int>(value.x));
				case ZephyrType::Bool:
					return std::string(value.x != 0.0f ? "true" : "false");
				default:
					return std::string();
				}
			};

		for (const EffectNode& node : emitter.nodes_)
		{
			const ZephyrNodeDefinition* definition = ZephyrNodeRegistry::Find(node.type_);

			canvas_.PlaceNode(node.id_, node.position_);
			canvas_.BeginNode(node.id_);
			if (!definition)
			{
				canvas_.NodeHeader(node.type_.c_str(), Color(0.55f, 0.15f, 0.15f, 1.0f));
				ImGui::TextDisabled("不明なノード");
				canvas_.EndNode();
				continue;
			}

			canvas_.NodeHeader(definition->label_.c_str(), definition->removable_ ? Color(0.20f, 0.35f, 0.55f, 1.0f) : Color(0.35f, 0.35f, 0.35f, 1.0f));

			ImGui::BeginGroup();
			for (Size inputIndex = 0;inputIndex < definition->inputs_.size();++inputIndex)
			{
				const ZephyrNodeInput& input = definition->inputs_[inputIndex];
				Bool connected = std::ranges::any_of(emitter.links_, [&node, &input](const EffectLink& link) { return link.toNode_ == node.id_ && link.toPin_ == input.name_; });

				Vector4 value = input.default_;
				auto found = std::ranges::find_if(node.parameters_, [&input](const EffectParameter& parameter) { return parameter.name_ == input.name_; });
				if (found != node.parameters_.end())
				{
					value = found->value_;
				}

				std::string text = connected ? std::format("{} 接続中", input.name_.c_str()) : std::format("{} {}", input.name_.c_str(), valueText(input.type_, value));
				if (input.pinned_)
				{
					canvas_.BeginPin(NodePin{ node.id_, static_cast<Uint32>(inputIndex) }, NodePinKind::Input, NodePinPlacement::Side);
					ImGui::TextUnformatted(text.c_str());
					canvas_.EndPin();
				}
				else
				{
					ImGui::Dummy(ImVec2(8.0f, ImGui::GetTextLineHeight()));
					ImGui::SameLine();
					ImGui::TextDisabled("%s", text.c_str());
				}
			}
			ImGui::EndGroup();

			if (!definition->outputs_.empty())
			{
				ImGui::SameLine();
				ImGui::BeginGroup();
				for (Size outputIndex = 0;outputIndex < definition->outputs_.size();++outputIndex)
				{
					canvas_.BeginPin(NodePin{ node.id_, static_cast<Uint32>(definition->inputs_.size() + outputIndex) }, NodePinKind::Output, NodePinPlacement::Side);
					ImGui::TextUnformatted(definition->outputs_[outputIndex].name_.c_str());
					canvas_.EndPin();
				}
				ImGui::EndGroup();
			}
			canvas_.EndNode();
		}
	}

	void EffectGraphPanel::DrawLinks(EffectEmitter& emitter)
	{
		for (const EffectLink& link : emitter.links_)
		{
			auto fromNode = std::ranges::find_if(emitter.nodes_, [&link](const EffectNode& node) { return node.id_ == link.fromNode_; });
			auto toNode = std::ranges::find_if(emitter.nodes_, [&link](const EffectNode& node) { return node.id_ == link.toNode_; });
			if (fromNode == emitter.nodes_.end() || toNode == emitter.nodes_.end())
			{
				continue;
			}

			const ZephyrNodeDefinition* fromDefinition = ZephyrNodeRegistry::Find(fromNode->type_);
			const ZephyrNodeDefinition* toDefinition = ZephyrNodeRegistry::Find(toNode->type_);
			if (!fromDefinition || !toDefinition)
			{
				continue;
			}

			auto output = std::ranges::find_if(fromDefinition->outputs_, [&link](const ZephyrNodeOutput& candidate) { return candidate.name_ == link.fromPin_; });
			auto input = std::ranges::find_if(toDefinition->inputs_, [&link](const ZephyrNodeInput& candidate) { return candidate.name_ == link.toPin_; });
			if (output == fromDefinition->outputs_.end() || input == toDefinition->inputs_.end())
			{
				continue;
			}

			Uint32 fromIndex = static_cast<Uint32>(fromDefinition->inputs_.size() + std::distance(fromDefinition->outputs_.begin(), output));
			Uint32 toIndex = static_cast<Uint32>(std::distance(toDefinition->inputs_.begin(), input));
			canvas_.Link(link.id_, NodePin{ link.fromNode_, fromIndex }, NodePin{ link.toNode_, toIndex }, NodeLinkShape::Curve, Color(1.0f, 1.0f, 1.0f, 0.7f));
		}
	}

	void EffectGraphPanel::DrawMenu(EffectEmitter& emitter)
	{
		if (!canvas_.BeginMenu(NodeTarget::Background))
		{
			return;
		}

		DynamicArray<String> categories;
		for (const ZephyrNodeDefinition& definition : ZephyrNodeRegistry::Nodes())
		{
			if (definition.removable_ && std::ranges::find(categories, definition.category_) == categories.end())
			{
				categories.push_back(definition.category_);
			}
		}

		for (const String& category : categories)
		{
			if (!ImGui::BeginMenu(category.c_str()))
			{
				continue;
			}
			for (const ZephyrNodeDefinition& definition : ZephyrNodeRegistry::Nodes())
			{
				if (!definition.removable_ || definition.category_ != category)
				{
					continue;
				}
				if (ImGui::MenuItem(definition.label_.c_str()))
				{
					Uint32 nodeID = 1;
					for (const EffectNode& existing : emitter.nodes_)
					{
						nodeID = Max(nodeID, existing.id_ + 1);
					}

					EffectNode& node = emitter.nodes_.emplace_back();
					node.id_ = nodeID;
					node.type_ = definition.type_;
					node.position_ = canvas_.MenuPosition();
					canvas_.PlaceNode(nodeID, node.position_);
					dirty_ = true;
				}
			}
			ImGui::EndMenu();
		}
		canvas_.EndMenu();
	}

	void EffectGraphPanel::UpdateConnect(EffectEmitter& emitter)
	{
		if (canvas_.BeginConnect())
		{
			NodePin from;
			NodePin to;
			if (canvas_.QueryConnectLink(from, to))
			{
				auto definitionOf = [&emitter](const NodePin& pin) -> const ZephyrNodeDefinition*
					{
						auto found = std::ranges::find_if(emitter.nodes_, [&pin](const EffectNode& node) { return node.id_ == pin.node_; });
						if (found == emitter.nodes_.end())
						{
							return nullptr;
						}
						return ZephyrNodeRegistry::Find(found->type_);
					};
								const ZephyrNodeDefinition* fromDefinition = definitionOf(from);
				const ZephyrNodeDefinition* toDefinition = definitionOf(to);
				if (fromDefinition && from.index_ < fromDefinition->inputs_.size())
				{
					std::swap(from, to);
					std::swap(fromDefinition, toDefinition);
				}

				Bool valid = fromDefinition && toDefinition && from.node_ != to.node_ && from.index_ >= fromDefinition->inputs_.size() && to.index_ < toDefinition->inputs_.size();
				if (valid)
				{
					const ZephyrNodeOutput& output = fromDefinition->outputs_[from.index_ - fromDefinition->inputs_.size()];
					const ZephyrNodeInput& input = toDefinition->inputs_[to.index_];
					Bool linked = std::ranges::any_of(emitter.links_, [&](const EffectLink& link) { return link.fromNode_ == from.node_ && link.fromPin_ == output.name_ && link.toNode_ == to.node_ && link.toPin_ == input.name_; });
					valid = !linked && ZephyrNodeRegistry::Connectable(output.type_, input.type_);
				}

				if (!valid)
				{
					canvas_.RejectConnect();
				}
				else if (canvas_.AcceptConnect())
				{
					const ZephyrNodeOutput& output = fromDefinition->outputs_[from.index_ - fromDefinition->inputs_.size()];
					const ZephyrNodeInput& input = toDefinition->inputs_[to.index_];
					if (!input.multiple_)
					{
						erase_if(emitter.links_, [&to, &input](const EffectLink& link) { return link.toNode_ == to.node_ && link.toPin_ == input.name_; });
					}

					Uint32 linkID = 1;
					for (const EffectLink& existing : emitter.links_)
					{
						linkID = Max(linkID, existing.id_ + 1);
					}

					EffectLink& link = emitter.links_.emplace_back();
					link.id_ = linkID;
					link.fromNode_ = from.node_;
					link.fromPin_ = output.name_;
					link.toNode_ = to.node_;
					link.toPin_ = input.name_;
					dirty_ = true;
				}
			}
		}
		canvas_.EndConnect();
	}

	void EffectGraphPanel::UpdateLayout(EffectEmitter& emitter)
	{
		for (EffectNode& node : emitter.nodes_)
		{
			Vector2 position = canvas_.NodePosition(node.id_);
			if (position != node.position_)
			{
				node.position_ = position;
				dirty_ = true;
			}
		}
	}
}