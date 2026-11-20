#include <Editor/Editor/Node/NodeCanvas.h>
#include <Editor/Editor/ImGui/ImGuiCommon.h>

namespace SeedCore
{
	NodeCanvas::NodeCanvas(const Vector4& padding, NodeLinkShape connecting)
	{
		ed::Config config;
		config.SettingsFile = nullptr;
		config.NavigateButtonIndex = 2;
		context_ = ed::CreateEditor(&config);
		ed::SetCurrentEditor(context_);
		ed::GetStyle().NodePadding = ImVec4(padding.x, padding.y, padding.z, padding.w);
		if (connecting == NodeLinkShape::Straight)
		{
			ed::GetStyle().LinkStrength = 0.0f;
		}
	}

	NodeCanvas::~NodeCanvas()
	{
		ed::DestroyEditor(context_);
	}

	void NodeCanvas::Begin(const Char* id)
	{
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImVec2 size = ImGui::GetContentRegionAvail();
		view_.min_ = Vector2(origin.x, origin.y);
		view_.max_ = Vector2(origin.x + size.x, origin.y + size.y);

		ed::SetCurrentEditor(context_);
		ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), true);
		ed::Begin(id, ImVec2(0.0f, 0.0f));
		for (Straight& straight : straights_)
		{
			straight.alive_ = false;
		}
		reselected_ = false;
		drafts_.clear();
	}

	void NodeCanvas::End()
	{
		ed::End();
		SeedCore::erase_if(straights_, [](const Straight& straight) { return !straight.alive_; });

		auto clip = [](const Vector2& from, const Vector2& to, const Vector2& rectMin, const Vector2& rectMax, Bool exit)
			{
				Vector2 delta = to - from;
				StaticArray<Float, 4> directions = { -delta.x, delta.x, -delta.y, delta.y };
				StaticArray<Float, 4> distances = { from.x - rectMin.x, rectMax.x - from.x, from.y - rectMin.y, rectMax.y - from.y };
				Float enter = 0.0f;
				Float leave = 1.0f;
				for (Size edgeIndex = 0;edgeIndex < 4;++edgeIndex)
				{
					if (Abs(directions[edgeIndex]) < 0.0001f)
					{
						if (distances[edgeIndex] < 0.0f)
						{
							return exit ? from : to;
						}
						continue;
					}
					Float ratio = distances[edgeIndex] / directions[edgeIndex];
					if (directions[edgeIndex] < 0.0f)
					{
						enter = Max(enter, ratio);
					}
					else
					{
						leave = Min(leave, ratio);
					}
				}
				if (enter > leave)
				{
					return exit ? from : to;
				}
				return from + delta * (exit ? leave : enter);
			};

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		auto arrow = [drawList](const Vector2& start, const Vector2& finish, ImU32 color, Float thickness)
			{
				Vector2 line = finish - start;
				Float length = line.Length();
				if (length < 1.0f)
				{
					return;
				}
				Vector2 direction = line / length;
				Vector2 middle = (start + finish) * 0.5f;
				Vector2 side = Vector2(-direction.y, direction.x);
				Vector2 tip = middle + direction * 6.0f;
				Vector2 back = middle - direction * 6.0f;
				Vector2 left = back + side * 4.0f;
				Vector2 right = back - side * 4.0f;
				drawList->AddLine(ImVec2(start.x, start.y), ImVec2(back.x, back.y), color, thickness);
				drawList->AddLine(ImVec2(tip.x, tip.y), ImVec2(finish.x, finish.y), color, thickness);
				drawList->AddTriangleFilled(ImVec2(tip.x, tip.y), ImVec2(left.x, left.y), ImVec2(right.x, right.y), color);
			};

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::GetIO().MouseDragMaxDistanceSqr[0] <= ImGui::GetIO().MouseDragThreshold * ImGui::GetIO().MouseDragThreshold)
		{
			Bool append = ImGui::GetIO().KeyCtrl;
			Uint32 hoveredID = ed::GetHoveredNode() ? 0 : HoveredLink();
			for (Straight& straight : straights_)
			{
				Bool selected = straight.selected_;
				if (straight.link_ == hoveredID)
				{
					selected = append ? !straight.selected_ : true;
				}
				else if (!append)
				{
					selected = false;
				}
				if (selected != straight.selected_)
				{
					straight.selected_ = selected;
					reselected_ = true;
				}
			}
		}

		for (Size straightIndex = 0;straightIndex < straights_.size();++straightIndex)
		{
			Straight& straight = straights_[straightIndex];
			ImVec2 fromPosition = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(straight.from_.node_)));
			ImVec2 fromSize = ed::GetNodeSize(ed::NodeId(static_cast<uintptr_t>(straight.from_.node_)));
			ImVec2 toPosition = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(straight.to_.node_)));
			ImVec2 toSize = ed::GetNodeSize(ed::NodeId(static_cast<uintptr_t>(straight.to_.node_)));
			Vector2 fromMin = Vector2(fromPosition.x, fromPosition.y);
			Vector2 fromMax = Vector2(fromPosition.x + fromSize.x, fromPosition.y + fromSize.y);
			Vector2 toMin = Vector2(toPosition.x, toPosition.y);
			Vector2 toMax = Vector2(toPosition.x + toSize.x, toPosition.y + toSize.y);

			Bool fromAnchored = (straight.fromAnchor_.x != 0.0f || straight.fromAnchor_.y != 0.0f) && straight.fromAnchor_.x >= 0.0f && straight.fromAnchor_.x <= fromSize.x && straight.fromAnchor_.y >= 0.0f && straight.fromAnchor_.y <= fromSize.y;
			Bool toAnchored = (straight.toAnchor_.x != 0.0f || straight.toAnchor_.y != 0.0f) && straight.toAnchor_.x >= 0.0f && straight.toAnchor_.x <= toSize.x && straight.toAnchor_.y >= 0.0f && straight.toAnchor_.y <= toSize.y;
			Vector2 fromPoint = fromAnchored ? fromMin + straight.fromAnchor_ : (fromMin + fromMax) * 0.5f;
			Vector2 toPoint = toAnchored ? toMin + straight.toAnchor_ : (toMin + toMax) * 0.5f;

			Uint32 pairLow = Min(straight.from_.node_, straight.to_.node_);
			Uint32 pairHigh = Max(straight.from_.node_, straight.to_.node_);
			Size pairOrdinal = 0;
			Size pairCount = 0;
			for (Size otherIndex = 0;otherIndex < straights_.size();++otherIndex)
			{
				const Straight& other = straights_[otherIndex];
				if (Min(other.from_.node_, other.to_.node_) != pairLow || Max(other.from_.node_, other.to_.node_) != pairHigh)
				{
					continue;
				}
				if (otherIndex < straightIndex)
				{
					++pairOrdinal;
				}
				++pairCount;
			}

			Vector2 delta = toPoint - fromPoint;
			if (pairCount > 1 && delta.Length() > 0.0001f)
			{
				Vector2 perpendicular = Vector2(-delta.y, delta.x) / delta.Length();
				Float fromThickness = Abs(perpendicular.x) * fromSize.x + Abs(perpendicular.y) * fromSize.y;
				Float toThickness = Abs(perpendicular.x) * toSize.x + Abs(perpendicular.y) * toSize.y;
				Float spacing = Min(14.0f, Min(fromThickness, toThickness) * 0.8f / (static_cast<Float>(pairCount) - 1.0f));
				Float offset = (static_cast<Float>(pairOrdinal) - (static_cast<Float>(pairCount) - 1.0f) * 0.5f) * spacing;
				if (straight.from_.node_ > straight.to_.node_)
				{
					offset = -offset;
				}
				fromPoint += perpendicular * offset;
				toPoint += perpendicular * offset;
			}

			Vector2 clippedFrom = clip(fromPoint, toPoint, fromMin, fromMax, true);
			Vector2 clippedTo = clip(fromPoint, toPoint, toMin, toMax, false);
			ImVec2 screenFrom = ed::CanvasToScreen(ImVec2(clippedFrom.x, clippedFrom.y));
			ImVec2 screenTo = ed::CanvasToScreen(ImVec2(clippedTo.x, clippedTo.y));
			Vector2 start = Vector2(screenFrom.x, screenFrom.y);
			Vector2 finish = Vector2(screenTo.x, screenTo.y);
			Vector2 line = finish - start;
			Float length = line.Length();
			if (length < 1.0f)
			{
				continue;
			}

			straight.start_ = start;
			straight.finish_ = finish;

			ImU32 color = straight.selected_ ? IM_COL32(255, 176, 50, 255) : ImGui::ColorConvertFloat4ToU32(ImVec4(straight.color_.r, straight.color_.g, straight.color_.b, straight.color_.a));
			Float thickness = straight.selected_ ? 3.0f : 2.0f;
			arrow(start, finish, color, thickness);
		}

		for (const Draft& draft : drafts_)
		{
			ImVec2 position = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(draft.from_.node_)));
			ImVec2 size = ed::GetNodeSize(ed::NodeId(static_cast<uintptr_t>(draft.from_.node_)));
			Vector2 nodeMin = Vector2(position.x, position.y);
			Vector2 nodeMax = Vector2(position.x + size.x, position.y + size.y);

			Bool anchored = (draft.fromAnchor_.x != 0.0f || draft.fromAnchor_.y != 0.0f) && draft.fromAnchor_.x >= 0.0f && draft.fromAnchor_.x <= size.x && draft.fromAnchor_.y >= 0.0f && draft.fromAnchor_.y <= size.y;
			Vector2 fromPoint = anchored ? nodeMin + draft.fromAnchor_ : (nodeMin + nodeMax) * 0.5f;
			Vector2 clippedFrom = clip(fromPoint, draft.toAnchor_, nodeMin, nodeMax, true);

			ImVec2 screenFrom = ed::CanvasToScreen(ImVec2(clippedFrom.x, clippedFrom.y));
			ImVec2 screenTo = ed::CanvasToScreen(ImVec2(draft.toAnchor_.x, draft.toAnchor_.y));
			arrow(Vector2(screenFrom.x, screenFrom.y), Vector2(screenTo.x, screenTo.y), ImGui::ColorConvertFloat4ToU32(ImVec4(draft.color_.r, draft.color_.g, draft.color_.b, draft.color_.a)), 2.0f);
		}

		ImGui::EndChild();
	}

	void NodeCanvas::BeginNode(Uint32 nodeID, const Color& background, const Color& border)
	{
		header_.node_ = nodeID;
		header_.active_ = false;
		ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(background.r, background.g, background.b, background.a));
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImVec4(border.r, border.g, border.b, border.a));
		ed::BeginNode(ed::NodeId(static_cast<uintptr_t>(nodeID)));
		ImGui::PushID(static_cast<Int>(nodeID));
	}

	void NodeCanvas::EndNode()
	{
		ImGui::PopID();
		ed::EndNode();
		ed::PopStyleColor(2);
		if (!header_.active_)
		{
			return;
		}

		ed::NodeId nodeID = ed::NodeId(static_cast<uintptr_t>(header_.node_));
		ImVec2 position = ed::GetNodePosition(nodeID);
		ImVec2 size = ed::GetNodeSize(nodeID);
		ed::Style& style = ed::GetStyle();
		ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(header_.color_.r, header_.color_.g, header_.color_.b, header_.color_.a));
		ed::GetNodeBackgroundDrawList(nodeID)->AddRectFilled(position, ImVec2(position.x + size.x, header_.bottom_ + style.NodePadding.y), color, style.NodeRounding, ImDrawFlags_RoundCornersTop);
	}

	void NodeCanvas::NodeHeader(const Char* title, const Color& color)
	{
		ImGui::TextUnformatted(title);
		header_.active_ = true;
		header_.bottom_ = ImGui::GetItemRectMax().y;
		header_.color_ = color;
	}

	void NodeCanvas::BeginPin(const NodePin& pin, NodePinKind kind, NodePinPlacement placement)
	{
		pin_.kind_ = kind;
		pin_.placement_ = placement;
		ed::BeginPin(ed::PinId((static_cast<uintptr_t>(pin.node_) << 32) | pin.index_), kind == NodePinKind::Input ? ed::PinKind::Input : ed::PinKind::Output);
		if (placement == NodePinPlacement::Whole)
		{
			ed::PinPivotAlignment(ImVec2(0.0f, 0.0f));
			ed::PinPivotSize(ImVec2(-1.0f, -1.0f));
			return;
		}

		ed::PinPivotAlignment(ImVec2(kind == NodePinKind::Input ? 0.0f : 1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0.0f, 0.0f));
		if (kind == NodePinKind::Input)
		{
			ImGui::Dummy(ImVec2(8.0f, ImGui::GetTextLineHeight()));
			ImVec2 itemMin = ImGui::GetItemRectMin();
			ImVec2 itemMax = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f), 4.0f, IM_COL32(255, 255, 255, 255));
			ImGui::SameLine();
		}
	}

	void NodeCanvas::EndPin()
	{
		if (pin_.placement_ == NodePinPlacement::Side && pin_.kind_ == NodePinKind::Output)
		{
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(8.0f, ImGui::GetTextLineHeight()));
			ImVec2 itemMin = ImGui::GetItemRectMin();
			ImVec2 itemMax = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f), 4.0f, IM_COL32(255, 255, 255, 255));
		}
		ed::EndPin();
	}

	void NodeCanvas::Link(Uint32 linkID, const NodePin& from, const NodePin& to, NodeLinkShape shape, const Color& color, const Vector2& fromAnchor, const Vector2& toAnchor)
	{
		if (shape == NodeLinkShape::Curve)
		{
			ed::Link(ed::LinkId(static_cast<uintptr_t>(linkID)), ed::PinId((static_cast<uintptr_t>(from.node_) << 32) | from.index_), ed::PinId((static_cast<uintptr_t>(to.node_) << 32) | to.index_), ImVec4(color.r, color.g, color.b, color.a), 2.0f);
			return;
		}
		
		for (Straight& straight : straights_)
		{
			if (straight.link_ != linkID)
			{
				continue;
			}
			straight.from_ = from;
			straight.to_ = to;
			straight.fromAnchor_ = fromAnchor;
			straight.toAnchor_ = toAnchor;
			straight.color_ = color;
			straight.alive_ = true;
			return;
		}

		Straight straight = Straight{ linkID, from, to, fromAnchor, toAnchor, color };
		straight.alive_ = true;
		straights_.push_back(straight);
	}

	void NodeCanvas::Guide(const NodePin& from, const Vector2& to, const Color& color, const Vector2& fromAnchor)
	{
		drafts_.push_back(Draft{ from, fromAnchor, to, color });
	}

	Bool NodeCanvas::BeginConnect()
	{
		menu_.connecting_ = false;
		return ed::BeginCreate();
	}

	void NodeCanvas::EndConnect()
	{
		ed::EndCreate();
	}

	Bool NodeCanvas::QueryConnectLink(NodePin& from, NodePin& to)
	{
		ed::PinId fromID;
		ed::PinId toID;
		if (!ed::QueryNewLink(&fromID, &toID))
		{
			return false;
		}
		from = NodePin{ static_cast<Uint32>(fromID.Get() >> 32), static_cast<Uint32>(fromID.Get() & 0xFFFFFFFFull) };
		to = NodePin{ static_cast<Uint32>(toID.Get() >> 32), static_cast<Uint32>(toID.Get() & 0xFFFFFFFFull) };
		return true;
	}

	Bool NodeCanvas::QueryConnectLink(NodePin& from, NodePin& to, Vector2& fromAnchor, Vector2& toAnchor)
	{
		if (!QueryConnectLink(from, to))
		{
			return false;
		}
		ImVec2 press = ImGui::GetIO().MouseClickedPos[0];
		ImVec2 release = ImGui::GetMousePos();
		ImVec2 fromPosition = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(from.node_)));
		ImVec2 toPosition = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(to.node_)));
		fromAnchor = Vector2(press.x - fromPosition.x, press.y - fromPosition.y);
		toAnchor = Vector2(release.x - toPosition.x, release.y - toPosition.y);
		return true;
	}

	Bool NodeCanvas::QueryConnectNode(NodePin& pin)
	{
		ed::PinId pinID;
		if (!ed::QueryNewNode(&pinID))
		{
			return false;
		}
		pin = NodePin{ static_cast<Uint32>(pinID.Get() >> 32), static_cast<Uint32>(pinID.Get() & 0xFFFFFFFFull) };
		menu_.connecting_ = true;
		menu_.pin_ = pin;
		return true;
	}

	Bool NodeCanvas::AcceptConnect()
	{
		if (!ed::AcceptNewItem())
		{
			return false;
		}
		if (menu_.connecting_)
		{
			ImVec2 position = ImGui::GetMousePos();
			menu_.target_ = NodeTarget::Background;
			menu_.position_ = Vector2(position.x, position.y);
			ed::Suspend();
			ImGui::OpenPopup("##NodeCanvasBackground");
			ed::Resume();
		}
		return true;
	}

	void NodeCanvas::RejectConnect()
	{
		ed::RejectNewItem(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 2.0f);
	}

	Bool NodeCanvas::BeginRemove()
	{
		Bool deleting = ed::BeginDelete();
		remove_.pressed_ = std::ranges::any_of(straights_, [](const Straight& straight) { return straight.selected_; }) && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete);
		remove_.straight_ = false;
		remove_.index_ = 0;
		return deleting || remove_.pressed_;
	}

	void NodeCanvas::EndRemove()
	{
		ed::EndDelete();
	}

	Bool NodeCanvas::QueryRemoveLink(Uint32& linkID)
	{
		ed::LinkId removedID;
		if (ed::QueryDeletedLink(&removedID))
		{
			linkID = static_cast<Uint32>(removedID.Get());
			remove_.straight_ = false;
			return true;
		}
		if (!remove_.pressed_)
		{
			return false;
		}
		while (remove_.index_ < straights_.size() && !straights_[remove_.index_].selected_)
		{
			++remove_.index_;
		}
		if (remove_.index_ >= straights_.size())
		{
			return false;
		}
		linkID = straights_[remove_.index_].link_;
		remove_.straight_ = true;
		return true;
	}

	Bool NodeCanvas::QueryRemoveNode(Uint32& nodeID)
	{
		ed::NodeId removedID;
		if (!ed::QueryDeletedNode(&removedID))
		{
			return false;
		}
		nodeID = static_cast<Uint32>(removedID.Get());
		remove_.straight_ = false;
		return true;
	}

	Bool NodeCanvas::AcceptRemove()
	{
		if (!remove_.straight_)
		{
			return ed::AcceptDeletedItem();
		}
		straights_[remove_.index_].selected_ = false;
		++remove_.index_;
		return true;
	}

	void NodeCanvas::RejectRemove()
	{
		if (!remove_.straight_)
		{
			ed::RejectDeletedItem();
			return;
		}
		++remove_.index_;
	}

	Bool NodeCanvas::BeginMenu(NodeTarget target)
	{
		StaticArray<const Char*, 4> names = { "##NodeCanvasBackground", "##NodeCanvasNode", "##NodeCanvasPin", "##NodeCanvasLink" };
		const Char* name = names[static_cast<Size>(target)];
		ImVec2 mouse = ImGui::GetMousePos();

		ed::Suspend();
		Bool opened = false;
		if (target == NodeTarget::Background)
		{
			opened = ed::ShowBackgroundContextMenu() && HoveredLink() == 0;
			if (opened)
			{
				menu_.pin_ = NodePin{};
			}
		}
		else if (target == NodeTarget::Node)
		{
			ed::NodeId nodeID;
			opened = ed::ShowNodeContextMenu(&nodeID);
			if (opened)
			{
				menu_.node_ = static_cast<Uint32>(nodeID.Get());
			}
		}
		else if (target == NodeTarget::Pin)
		{
			ed::PinId pinID;
			opened = ed::ShowPinContextMenu(&pinID);
			if (opened)
			{
				menu_.pin_ = NodePin{ static_cast<Uint32>(pinID.Get() >> 32), static_cast<Uint32>(pinID.Get() & 0xFFFFFFFFull) };
			}
		}
		else
		{
			ed::LinkId linkID;
			if (ed::ShowLinkContextMenu(&linkID))
			{
				opened = true;
				menu_.link_ = static_cast<Uint32>(linkID.Get());
			}
			else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && HoveredLink() != 0)
			{
				opened = true;
				menu_.link_ = HoveredLink();
			}
		}

		if (opened)
		{
			menu_.target_ = target;
			menu_.position_ = Vector2(mouse.x, mouse.y);
			ImGui::OpenPopup(name);
		}
		if (ImGui::BeginPopup(name))
		{
			return true;
		}
		ed::Resume();
		return false;
	}

	void NodeCanvas::EndMenu()
	{
		ImGui::EndPopup();
		ed::Resume();
	}

	void NodeCanvas::Tooltip(const Char* text)
	{
		if (!ImGui::IsItemHovered())
		{
			return;
		}
		ed::Suspend();
		ImGui::SetTooltip("%s", text);
		ed::Resume();
	}

	Uint32 NodeCanvas::MenuNode()const
	{
		return menu_.node_;
	}

	NodePin NodeCanvas::MenuPin()const
	{
		return menu_.pin_;
	}

	Uint32 NodeCanvas::MenuLink()const
	{
		return menu_.link_;
	}

	Vector2 NodeCanvas::MenuPosition()const
	{
		return menu_.position_;
	}

	void NodeCanvas::PlaceNode(Uint32 nodeID, const Vector2& position)
	{
		ed::SetCurrentEditor(context_);
		ed::SetNodePosition(ed::NodeId(static_cast<uintptr_t>(nodeID)), ImVec2(position.x, position.y));
	}

	Vector2 NodeCanvas::NodePosition(Uint32 nodeID)const
	{
		ed::SetCurrentEditor(context_);
		ImVec2 position = ed::GetNodePosition(ed::NodeId(static_cast<uintptr_t>(nodeID)));
		return Vector2(position.x, position.y);
	}

	Vector2 NodeCanvas::NodeSize(Uint32 nodeID)const
	{
		ed::SetCurrentEditor(context_);
		ImVec2 size = ed::GetNodeSize(ed::NodeId(static_cast<uintptr_t>(nodeID)));
		return Vector2(size.x, size.y);
	}

	void NodeCanvas::SelectNode(Uint32 nodeID, Bool append)
	{
		ed::SetCurrentEditor(context_);
		if (!append)
		{
			for (Straight& straight : straights_)
			{
				straight.selected_ = false;
			}
		}
		ed::SelectNode(ed::NodeId(static_cast<uintptr_t>(nodeID)), append);
	}

	void NodeCanvas::SelectLink(Uint32 linkID, Bool append)
	{
		ed::SetCurrentEditor(context_);
		if (!append)
		{
			ed::ClearSelection();
			for (Straight& straight : straights_)
			{
				straight.selected_ = false;
			}
		}
		for (Straight& straight : straights_)
		{
			if (straight.link_ == linkID)
			{
				straight.selected_ = true;
				return;
			}
		}
		ed::SelectLink(ed::LinkId(static_cast<uintptr_t>(linkID)), true);
	}

	void NodeCanvas::Deselect()
	{
		ed::SetCurrentEditor(context_);
		ed::ClearSelection();
		for (Straight& straight : straights_)
		{
			straight.selected_ = false;
		}
	}

	Bool NodeCanvas::Reselected()const
	{
		ed::SetCurrentEditor(context_);
		return ed::HasSelectionChanged() || reselected_;
	}

	DynamicArray<Uint32> NodeCanvas::SelectedNodes()const
	{
		ed::SetCurrentEditor(context_);
		Int count = ed::GetSelectedObjectCount();
		DynamicArray<ed::NodeId> nodeIDs(static_cast<Size>(count));
		Int nodeCount = ed::GetSelectedNodes(nodeIDs.data(), count);
		DynamicArray<Uint32> selected;
		for (Int index = 0;index < nodeCount;++index)
		{
			selected.push_back(static_cast<Uint32>(nodeIDs[index].Get()));
		}
		return selected;
	}

	DynamicArray<Uint32> NodeCanvas::SelectedLinks()const
	{
		ed::SetCurrentEditor(context_);
		Int count = ed::GetSelectedObjectCount();
		DynamicArray<ed::LinkId> linkIDs(static_cast<Size>(count));
		Int linkCount = ed::GetSelectedLinks(linkIDs.data(), count);
		DynamicArray<Uint32> selected;
		for (Int index = 0;index < linkCount;++index)
		{
			selected.push_back(static_cast<Uint32>(linkIDs[index].Get()));
		}
		for (const Straight& straight : straights_)
		{
			if (straight.selected_)
			{
				selected.push_back(straight.link_);
			}
		}
		return selected;
	}

	Uint32 NodeCanvas::HoveredNode()const
	{
		ed::SetCurrentEditor(context_);
		return static_cast<Uint32>(ed::GetHoveredNode().Get());
	}

	NodePin NodeCanvas::HoveredPin()const
	{
		ed::SetCurrentEditor(context_);
		ed::PinId pinID = ed::GetHoveredPin();
		return NodePin{ static_cast<Uint32>(pinID.Get() >> 32), static_cast<Uint32>(pinID.Get() & 0xFFFFFFFFull) };
	}

	Uint32 NodeCanvas::HoveredLink()const
	{
		ed::SetCurrentEditor(context_);
		ed::LinkId hoveredID = ed::GetHoveredLink();
		if (hoveredID)
		{
			return static_cast<Uint32>(hoveredID.Get());
		}
		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			return 0;
		}

		Vector2 mouse = Vector2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
		Uint32 nearestID = 0;
		Float nearestDistance = 6.0f;
		for (const Straight& straight : straights_)
		{
			Vector2 line = straight.finish_ - straight.start_;
			Float lengthSq = line.LengthSq();
			if (lengthSq < 1.0f)
			{
				continue;
			}
			Float ratio = Clamp((mouse - straight.start_).Dot(line) / lengthSq, 0.0f, 1.0f);
			Float distance = (mouse - (straight.start_ + line * ratio)).Length();
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				nearestID = straight.link_;
			}
		}
		return nearestID;
	}

	Uint32 NodeCanvas::DoubleClickNode()const
	{
		ed::SetCurrentEditor(context_);
		return static_cast<Uint32>(ed::GetDoubleClickedNode().Get());
	}

	void NodeCanvas::FitAll()
	{
		ed::SetCurrentEditor(context_);
		ed::NavigateToContent();
	}

	void NodeCanvas::FitSelection()
	{
		ed::SetCurrentEditor(context_);
		ed::NavigateToSelection();
	}

	void NodeCanvas::CenterNode(Uint32 nodeID)
	{
		ed::SetCurrentEditor(context_);
		ed::CenterNodeOnScreen(ed::NodeId(static_cast<uintptr_t>(nodeID)));
	}

	Vector2 NodeCanvas::ViewCenter()const
	{
		ed::SetCurrentEditor(context_);
		Vector2 center = (view_.min_ + view_.max_) * 0.5f;
		ImVec2 canvas = ed::ScreenToCanvas(ImVec2(center.x, center.y));
		return Vector2(canvas.x, canvas.y);
	}

	Vector2 NodeCanvas::ScreenToCanvas(const Vector2& screen)const
	{
		ed::SetCurrentEditor(context_);
		ImVec2 canvas = ed::ScreenToCanvas(ImVec2(screen.x, screen.y));
		return Vector2(canvas.x, canvas.y);
	}

	Vector2 NodeCanvas::CanvasToScreen(const Vector2& canvas)const
	{
		ed::SetCurrentEditor(context_);
		ImVec2 screen = ed::CanvasToScreen(ImVec2(canvas.x, canvas.y));
		return Vector2(screen.x, screen.y);
	}

	Float NodeCanvas::Zoom()const
	{
		ed::SetCurrentEditor(context_);
		return ed::GetCurrentZoom();
	}
}