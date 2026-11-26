#include <Editor/Editor/Panel/Window/AnimatorControllerPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	AnimatorControllerPanel::AnimatorControllerPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), canvas_({ 0.0f, 0.0f, 0.0f, 0.0f }, NodeLinkShape::Straight)
	{
		/// No Code
	}

	void AnimatorControllerPanel::Draw()
	{
		if (!open_)
		{
			return;
		}

		Animator* selectedTarget = context_.selection_.Primary() ? const_cast<Animator*>(context_.selection_.Primary().GetComponent<Animator>()) : nullptr;
		if (selectedTarget != target_)
		{
			target_ = selectedTarget;
			selectedSourceIndex_ = -1;
			selectedConditionIndex_ = -1;
			canvas_.Deselect();
		}

		ImGuiWindow* editorViewWindow = ImGui::FindWindowByName("エディタービュー");
		ImGuiID dockID = (editorViewWindow && editorViewWindow->DockId != 0) ? editorViewWindow->DockId : context_.graphics_.imgui_->DockSpaceID();
		ImGui::DockBuilderDockWindow("アニメーターコントローラー", dockID);
		ImGui::SetNextWindowSize(ImVec2(1100, 650), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("アニメーターコントローラー", &open_))
		{
			if (!target_)
			{
				ImGui::TextDisabled("対象のAnimatorがありません");
			}
			else
			{
				DrawNodeEditor();
			}
		}
		ImGui::End();
	}

	void AnimatorControllerPanel::Open(Animator* target)
	{
		open_ = true;
		ImGui::SetWindowFocus("アニメーターコントローラー");

		if (target != target_)
		{
			target_ = target;
			selectedSourceIndex_ = -1;
			selectedConditionIndex_ = -1;
			canvas_.Deselect();
		}
	}

	void AnimatorControllerPanel::Inspect()
	{
		if (!target_)
		{
			ImGui::TextDisabled("対象のAnimatorがありません");
			return;
		}

		ImGui::TextDisabled("パラメータ");
		ImGui::Separator();

		{
			Size removeParameterIndex = SIZE_MAX;

			for (Size parameterIndex = 0; parameterIndex < target_->parameters_.size(); ++parameterIndex)
			{
				AnimationParameter& parameter = target_->parameters_[parameterIndex];

				ImGui::PushID(static_cast<Int>(parameterIndex));

				std::string parameterNameBuffer = parameter.name_.str();
				parameterNameBuffer.resize(64);
				ImGui::SetNextItemWidth(100.0f);
				if (ImGui::InputText("##parameterName", parameterNameBuffer.data(), parameterNameBuffer.capacity()))
				{
					parameter.name_ = String(std::string_view(parameterNameBuffer.c_str()));
				}

				ImGui::SameLine();

				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::BeginCombo("##parameterType", ParameterLabel(parameter.type_)))
				{
					for (Int typeValue = 0; typeValue <= static_cast<Int>(AnimationParameterType::Trigger); ++typeValue)
					{
						AnimationParameterType type = static_cast<AnimationParameterType>(typeValue);
						Bool selected = (parameter.type_ == type);
						if (ImGui::Selectable(ParameterLabel(type), selected))
						{
							parameter.type_ = type;
						}
					}
					ImGui::EndCombo();
				}

				ImGui::SameLine();

				ImGui::SetNextItemWidth(60.0f);
				switch (parameter.type_)
				{
				case AnimationParameterType::Bool:
				{
					Bool boolValue = parameter.value_ != 0.0f;
					if (ImGui::Checkbox("##parameterValue", &boolValue))
					{
						parameter.value_ = boolValue ? 1.0f : 0.0f;
					}
					break;
				}
				case AnimationParameterType::Trigger:
				{
					if (ImGui::RadioButton("##parameterValue", parameter.value_ != 0.0f))
					{
						parameter.value_ = (parameter.value_ != 0.0f) ? 0.0f : 1.0f;
					}
					break;
				}
				case AnimationParameterType::Int:
				{
					Int intValue = static_cast<Int>(parameter.value_);
					if (ImGui::DragInt("##parameterValue", &intValue, 1.0f))
					{
						parameter.value_ = static_cast<Float>(intValue);
					}
					break;
				}
				default:
				{
					ImGui::DragFloat("##parameterValue", &parameter.value_, 0.01f);
					break;
				}
				}

				ImGui::SameLine();

				Float iconHeight = ImGui::GetTextLineHeight();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				Bool removeClicked = ImGui::ImageButton("##remove", static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Cross"))), ImVec2(iconHeight, iconHeight));
				ImGui::PopStyleColor();
				if (removeClicked)
				{
					removeParameterIndex = parameterIndex;
				}

				ImGui::PopID();
			}

			if (removeParameterIndex != SIZE_MAX)
			{
				target_->parameters_.erase(target_->parameters_.begin() + removeParameterIndex);
			}

			if (ImGui::Button("パラメータを追加", ImVec2(-1.0f, 0.0f)))
			{
				String uniqueName;
				for (Int suffix = 0; ; ++suffix)
				{
					std::string candidate = suffix == 0 ? std::string("Parameter") : ("Parameter" + std::to_string(suffix));
					String candidateName = String(std::string_view(candidate));

					Bool candidateExists = false;
					for (const AnimationParameter& parameter : target_->parameters_)
					{
						if (parameter.name_ == candidateName)
						{
							candidateExists = true;
							break;
						}
					}

					if (!candidateExists)
					{
						uniqueName = candidateName;
						break;
					}
				}

				AnimationParameter& newParameter = target_->parameters_.emplace_back();
				newParameter.name_ = uniqueName;
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("詳細");
		ImGui::Separator();

		Size transitionIndex = SIZE_MAX;
		for (Uint32 linkID : canvas_.SelectedLinks())
		{
			if (linkID >= 1 && linkID <= target_->transitions_.size())
			{
				transitionIndex = static_cast<Size>(linkID - 1);
				break;
			}
		}

		Size stateIndex = SIZE_MAX;
		for (Uint32 nodeID : canvas_.SelectedNodes())
		{
			if (nodeID >= 1 && nodeID <= target_->states_.size())
			{
				stateIndex = static_cast<Size>(nodeID - 1);
				break;
			}
		}

		if (transitionIndex != SIZE_MAX)
		{
			AnimationTransition& transition = target_->transitions_[transitionIndex];

			Size fromIndex = static_cast<Size>(transition.fromState_);
			Size toIndex = static_cast<Size>(transition.toState_);
			const Char* fromName = transition.fromState_ == Animator::AnyState ? "Any" : (fromIndex < target_->states_.size() ? target_->states_[fromIndex].name_.c_str() : "?");
			const Char* toName = toIndex < target_->states_.size() ? target_->states_[toIndex].name_.c_str() : "?";

			ImGui::Text("%s から %s", fromName, toName);
			ImGui::Spacing();

			ImGui::DragFloat("遷移時間", &transition.duration_, 0.01f, 0.0f, FLT_MAX);

			ImGui::Checkbox("Exit Timeを使用", &transition.hasExitTime_);
			if (transition.hasExitTime_)
			{
				ImGui::DragFloat("Exit Time", &transition.exitTime_, 0.01f, 0.0f, FLT_MAX);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextDisabled("条件");

			Size removeIndex = SIZE_MAX;

			for (Size conditionIndex = 0; conditionIndex < transition.conditions_.size(); ++conditionIndex)
			{
				AnimationCondition& condition = transition.conditions_[conditionIndex];

				ImGui::PushID(static_cast<Int>(conditionIndex));

				Bool isSelected = (selectedConditionIndex_ == static_cast<Int>(conditionIndex));
				ImGui::PushStyleColor(ImGuiCol_ChildBg, isSelected ? ImVec4(0.35f, 0.25f, 0.1f, 0.4f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_Border, isSelected ? ImVec4(1.0f, 0.69f, 0.2f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 0.25f));

				Float boxHeight = ImGui::GetFrameHeightWithSpacing() * (conditionIndex > 0 ? 4.0f : 3.0f) + 8.0f;
				ImGui::BeginChild("##conditionBox", ImVec2(0.0f, boxHeight), true);

				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					selectedConditionIndex_ = static_cast<Int>(conditionIndex);
				}
				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					selectedConditionIndex_ = static_cast<Int>(conditionIndex);
					ImGui::OpenPopup("##conditionContext");
				}
				if (ImGui::BeginPopup("##conditionContext"))
				{
					if (ImGui::MenuItem("削除"))
					{
						removeIndex = conditionIndex;
					}
					ImGui::EndPopup();
				}
				if (isSelected && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))
				{
					removeIndex = conditionIndex;
				}

				if (conditionIndex > 0)
				{
					const Char* logicLabel = condition.isOr_ ? "OR" : "AND";
					ImGui::SetNextItemWidth(120.0f);
					if (ImGui::BeginCombo("##logic", logicLabel))
					{
						if (ImGui::Selectable("AND", !condition.isOr_))
						{
							condition.isOr_ = false;
						}
						if (ImGui::Selectable("OR", condition.isOr_))
						{
							condition.isOr_ = true;
						}
						ImGui::EndCombo();
					}
				}

				std::string parameterPreview = condition.parameterName_.str();
				if (parameterPreview.empty())
				{
					parameterPreview = "(未選択)";
				}

				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::BeginCombo("##parameter", parameterPreview.c_str()))
				{
					for (const AnimationParameter& parameter : target_->parameters_)
					{
						std::string parameterName = parameter.name_.str();
						Bool selected = (condition.parameterName_ == parameter.name_);
						if (ImGui::Selectable(parameterName.c_str(), selected))
						{
							condition.parameterName_ = parameter.name_;
						}
					}
					ImGui::EndCombo();
				}

				const AnimationParameter* parameter = nullptr;
				for (const AnimationParameter& candidateParameter : target_->parameters_)
				{
					if (candidateParameter.name_ == condition.parameterName_)
					{
						parameter = &candidateParameter;
						break;
					}
				}
				AnimationParameterType parameterType = parameter ? parameter->type_ : AnimationParameterType::Float;

				if (parameterType != AnimationParameterType::Trigger)
				{
					if (parameterType == AnimationParameterType::Bool)
					{
						Bool boolValue = condition.value_ != 0.0f;
						if (ImGui::Checkbox("値", &boolValue))
						{
							condition.value_ = boolValue ? 1.0f : 0.0f;
							condition.comparison_ = AnimationConditionComparison::Equal;
						}
					}
					else
					{
						ImGui::SetNextItemWidth(120.0f);
						if (ImGui::BeginCombo("##comparison", ComparisonLabel(condition.comparison_)))
						{
							for (Int comparisonValue = 0; comparisonValue <= static_cast<Int>(AnimationConditionComparison::LessOrEqual); ++comparisonValue)
							{
								AnimationConditionComparison comparison = static_cast<AnimationConditionComparison>(comparisonValue);
								Bool selected = (condition.comparison_ == comparison);
								if (ImGui::Selectable(ComparisonLabel(comparison), selected))
								{
									condition.comparison_ = comparison;
								}
							}
							ImGui::EndCombo();
						}

						ImGui::SetNextItemWidth(120.0f);
						if (parameterType == AnimationParameterType::Int)
						{
							Int intValue = static_cast<Int>(condition.value_);
							if (ImGui::DragInt("##value", &intValue, 1.0f))
							{
								condition.value_ = static_cast<Float>(intValue);
							}
						}
						else
						{
							ImGui::DragFloat("##value", &condition.value_, 0.01f);
						}
					}
				}

				ImGui::EndChild();

				ImGui::PopStyleColor(2);

				ImGui::PopID();
			}

			if (removeIndex != SIZE_MAX)
			{
				transition.conditions_.erase(transition.conditions_.begin() + removeIndex);
				if (selectedConditionIndex_ == static_cast<Int>(removeIndex))
				{
					selectedConditionIndex_ = -1;
				}
			}

			if (ImGui::Button("条件を追加", ImVec2(-1.0f, 0.0f)))
			{
				transition.conditions_.emplace_back();
			}
		}
		else if (stateIndex != SIZE_MAX)
		{
			AnimationState& state = target_->states_[stateIndex];

			std::string nameBuffer = state.name_.str();
			nameBuffer.resize(128);
			if (ImGui::InputText("名前", nameBuffer.data(), nameBuffer.capacity()))
			{
				state.name_ = String(std::string_view(nameBuffer.c_str()));
			}
			ImGui::Spacing();

			if (ImGui::BeginCombo("Animation", AssetLabel(state.animationID_)))
			{
				for (Uint32 animationId : target_->animationIDs_)
				{
					Bool selected = (state.animationID_ == static_cast<Int>(animationId));
					if (ImGui::Selectable(AssetLabel(static_cast<Int>(animationId)), selected))
					{
						state.animationID_ = static_cast<Int>(animationId);
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Spacing();

			ImGui::Checkbox("ループ", &state.loop_);

			ImGui::Checkbox("ルートモーションを使用", &state.useRootMotion_);
		}
		else
		{
			ImGui::TextDisabled("ノードを選択してください");
		}
	}

	NODISCARD Bool AnimatorControllerPanel::Focused()const
	{
		ImGuiWindow* window = ImGui::FindWindowByName("アニメーターコントローラー");
		return open_ && window && !window->SkipItems;
	}

	void AnimatorControllerPanel::DrawNodeEditor()
	{
		if (ImGui::Button("ステートを追加"))
		{
			AnimationState& state = target_->states_.emplace_back();
			state.name_ = String("State");
			Vector2 center = canvas_.ViewCenter();
			state.nodePositionX_ = center.x - 100.0f;
			state.nodePositionY_ = center.y - 16.0f;
		}

		Bool removed = false;
		auto removeState = [&](Size stateIndex)
			{
				removed = true;
				target_->states_.erase(target_->states_.begin() + stateIndex);

				SeedCore::erase_if(target_->transitions_, [stateIndex](const AnimationTransition& transition) { return transition.fromState_ == static_cast<Int>(stateIndex) || transition.toState_ == static_cast<Int>(stateIndex); });

				for (AnimationTransition& transition : target_->transitions_)
				{
					if (transition.fromState_ > static_cast<Int>(stateIndex))
					{
						--transition.fromState_;
					}
					if (transition.toState_ > static_cast<Int>(stateIndex))
					{
						--transition.toState_;
					}
				}

				if (target_->entryStateIndex_ == static_cast<Int>(stateIndex))
				{
					target_->entryStateIndex_ = -1;
				}
				else if (target_->entryStateIndex_ > static_cast<Int>(stateIndex))
				{
					--target_->entryStateIndex_;
				}

				if (selectedSourceIndex_ == static_cast<Int>(stateIndex))
				{
					selectedSourceIndex_ = -1;
				}
				else if (selectedSourceIndex_ > static_cast<Int>(stateIndex))
				{
					--selectedSourceIndex_;
				}

				canvas_.Deselect();
				selectedConditionIndex_ = -1;
			};

		canvas_.Begin("AnimatorControllerEditor");

		if (ImGui::GetIO().KeyAlt)
		{
			altActive_ = true;
		}
		else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			altActive_ = false;
		}

		for (Size index = 0; index < target_->states_.size(); ++index)
		{
			AnimationState& state = target_->states_[index];
			Uint32 nodeID = static_cast<Uint32>(index + 1);
			Bool isDefaultState = (target_->entryStateIndex_ == static_cast<Int>(index));

			canvas_.PlaceNode(nodeID, Vector2(state.nodePositionX_, state.nodePositionY_));
			if (isDefaultState)
			{
				canvas_.BeginNode(nodeID, Color(0.62f, 0.35f, 0.08f, 1.0f), Color(1.0f, 0.68f, 0.22f, 1.0f));
			}
			else
			{
				canvas_.BeginNode(nodeID);
			}
			if (altActive_)
			{
				canvas_.BeginPin(NodePin{ nodeID, 0 }, NodePinKind::Output, NodePinPlacement::Whole);
			}
			ImGui::Dummy(ImVec2(200.0f, 8.0f));
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Max(0.0f, (200.0f - ImGui::CalcTextSize(state.name_.c_str()).x) * 0.5f));
			ImGui::TextUnformatted(state.name_.c_str());
			ImGui::Dummy(ImVec2(200.0f, 8.0f));
			if (altActive_)
			{
				canvas_.EndPin();
			}
			canvas_.EndNode();
		}

		auto specialNode = [&](Uint32 nodeID, const Char* title, const Color& background, const Color& border, Bool pinned, Float& positionX, Float& positionY)
			{
				canvas_.PlaceNode(nodeID, Vector2(positionX, positionY));
				canvas_.BeginNode(nodeID, background, border);
				if (pinned)
				{
					canvas_.BeginPin(NodePin{ nodeID, 0 }, NodePinKind::Output, NodePinPlacement::Whole);
				}
				ImGui::Dummy(ImVec2(100.0f, 8.0f));
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Max(0.0f, (100.0f - ImGui::CalcTextSize(title).x) * 0.5f));
				ImGui::TextUnformatted(title);
				ImGui::Dummy(ImVec2(100.0f, 8.0f));
				if (pinned)
				{
					canvas_.EndPin();
				}
				canvas_.EndNode();
			};

		specialNode(entryNodeID_, "Entry", Color(0.12f, 0.35f, 0.12f, 1.0f), Color(0.35f, 0.9f, 0.35f, 1.0f), false, target_->entryNodePositionX_, target_->entryNodePositionY_);
		specialNode(exitNodeID_, "Exit", Color(0.35f, 0.12f, 0.12f, 1.0f), Color(0.9f, 0.35f, 0.35f, 1.0f), altActive_, target_->exitNodePositionX_, target_->exitNodePositionY_);
		specialNode(anyNodeID_, "Any", Color(0.12f, 0.16f, 0.35f, 1.0f), Color(0.35f, 0.55f, 0.9f, 1.0f), altActive_, target_->anyNodePositionX_, target_->anyNodePositionY_);

		for (Size index = 0; index < target_->transitions_.size(); ++index)
		{
			const AnimationTransition& transition = target_->transitions_[index];
			Bool fromIsAny = (transition.fromState_ == Animator::AnyState);
			Bool toIsExit = (transition.toState_ == Animator::ExitState);
			Bool fromValid = fromIsAny || (transition.fromState_ >= 0 && static_cast<Size>(transition.fromState_) < target_->states_.size());
			Bool toValid = toIsExit || (transition.toState_ >= 0 && static_cast<Size>(transition.toState_) < target_->states_.size());
			if (!fromValid || !toValid || transition.fromState_ == transition.toState_)
			{
				continue;
			}

			Uint32 fromNodeID = fromIsAny ? anyNodeID_ : static_cast<Uint32>(transition.fromState_ + 1);
			Uint32 toNodeID = toIsExit ? exitNodeID_ : static_cast<Uint32>(transition.toState_ + 1);
			Color color = toIsExit ? Color(0.9f, 0.35f, 0.35f, 0.78f) : (fromIsAny ? Color(0.35f, 0.55f, 0.9f, 0.78f) : Color(1.0f, 1.0f, 1.0f, 0.7f));
			canvas_.Link(static_cast<Uint32>(index + 1), NodePin{ fromNodeID, 0 }, NodePin{ toNodeID, 0 }, NodeLinkShape::Straight, color, Vector2(transition.fromOffsetX_, transition.fromOffsetY_), Vector2(transition.toOffsetX_, transition.toOffsetY_));
		}

		if (target_->entryStateIndex_ >= 0 && static_cast<Size>(target_->entryStateIndex_) < target_->states_.size())
		{
			Uint32 entryTargetID = static_cast<Uint32>(target_->entryStateIndex_ + 1);
			Vector2 targetSize = canvas_.NodeSize(entryTargetID);
			Vector2 targetCenter = canvas_.NodePosition(entryTargetID) + targetSize * 0.5f;
			Vector2 entryCenter = canvas_.NodePosition(entryNodeID_) + canvas_.NodeSize(entryNodeID_) * 0.5f;
			Vector2 delta = entryCenter - targetCenter;
			Float scale = 1.0f;
			if (Abs(delta.x) > 0.0001f)
			{
				scale = Min(scale, targetSize.x * 0.5f / Abs(delta.x));
			}
			if (Abs(delta.y) > 0.0001f)
			{
				scale = Min(scale, targetSize.y * 0.5f / Abs(delta.y));
			}
			canvas_.Guide(NodePin{ entryNodeID_, 0 }, targetCenter + delta * scale, Color(0.39f, 0.86f, 0.39f, 0.86f));
		}

		if (canvas_.BeginConnect())
		{
			NodePin from;
			NodePin to;
			Vector2 fromAnchor = Vector2::Zero;
			Vector2 toAnchor = Vector2::Zero;
			if (canvas_.QueryConnectLink(from, to, fromAnchor, toAnchor) && canvas_.AcceptConnect())
			{
				Int fromState = -1;
				if (from.node_ == anyNodeID_)
				{
					fromState = Animator::AnyState;
				}
				else if (from.node_ >= 1 && from.node_ <= target_->states_.size())
				{
					fromState = static_cast<Int>(from.node_ - 1);
				}

				Int toState = -1;
				if (to.node_ == exitNodeID_)
				{
					toState = Animator::ExitState;
				}
				else if (to.node_ >= 1 && to.node_ <= target_->states_.size())
				{
					toState = static_cast<Int>(to.node_ - 1);
				}

				if (fromState != -1 && toState != -1 && fromState != toState)
				{
					AnimationTransition& transition = target_->transitions_.emplace_back();
					transition.fromState_ = fromState;
					transition.toState_ = toState;
					transition.fromOffsetX_ = fromAnchor.x;
					transition.fromOffsetY_ = fromAnchor.y;
					transition.toOffsetX_ = toAnchor.x;
					transition.toOffsetY_ = toAnchor.y;
				}
			}
		}
		canvas_.EndConnect();

		DynamicArray<Size> removedTransitions;
		DynamicArray<Size> removedStates;
		if (canvas_.BeginRemove())
		{
			Uint32 linkID = 0;
			while (canvas_.QueryRemoveLink(linkID))
			{
				if (linkID == 0 || linkID > target_->transitions_.size())
				{
					canvas_.RejectRemove();
					continue;
				}
				if (canvas_.AcceptRemove())
				{
					removedTransitions.push_back(static_cast<Size>(linkID - 1));
				}
			}

			Uint32 nodeID = 0;
			while (canvas_.QueryRemoveNode(nodeID))
			{
				if (nodeID >= 1 && nodeID <= target_->states_.size())
				{
					removedStates.push_back(static_cast<Size>(nodeID - 1));
				}
				canvas_.RejectRemove();
			}
		}
		canvas_.EndRemove();

		if (!removedTransitions.empty())
		{
			std::ranges::sort(removedTransitions, std::greater<Size>());
			for (Size transitionIndex : removedTransitions)
			{
				target_->transitions_.erase(target_->transitions_.begin() + transitionIndex);
			}
			canvas_.Deselect();
			selectedConditionIndex_ = -1;
		}

		std::ranges::sort(removedStates, std::greater<Size>());
		for (Size stateIndex : removedStates)
		{
			removeState(stateIndex);
		}

		if (selectedSourceIndex_ != -1)
		{
			Uint32 sourceNodeID = (selectedSourceIndex_ == Animator::AnyState) ? anyNodeID_ : static_cast<Uint32>(selectedSourceIndex_ + 1);
			Vector2 sourcePosition = canvas_.NodePosition(sourceNodeID);
			Vector2 from = canvas_.MenuPosition();
			Vector2 mouse = Vector2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);

			if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				selectedSourceIndex_ = -1;
			}
			else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				Int targetState = -1;
				Uint32 targetNodeID = 0;

				Vector2 exitPosition = canvas_.NodePosition(exitNodeID_);
				Vector2 exitSize = canvas_.NodeSize(exitNodeID_);
				if (mouse.x >= exitPosition.x && mouse.x <= exitPosition.x + exitSize.x && mouse.y >= exitPosition.y && mouse.y <= exitPosition.y + exitSize.y)
				{
					targetState = Animator::ExitState;
					targetNodeID = exitNodeID_;
				}

				if (targetState == -1)
				{
					for (Size candidate = 0; candidate < target_->states_.size(); ++candidate)
					{
						if (static_cast<Int>(candidate) == selectedSourceIndex_)
						{
							continue;
						}

						Uint32 candidateNodeID = static_cast<Uint32>(candidate + 1);
						Vector2 candidatePosition = canvas_.NodePosition(candidateNodeID);
						Vector2 candidateSize = canvas_.NodeSize(candidateNodeID);
						if (mouse.x >= candidatePosition.x && mouse.x <= candidatePosition.x + candidateSize.x && mouse.y >= candidatePosition.y && mouse.y <= candidatePosition.y + candidateSize.y)
						{
							targetState = static_cast<Int>(candidate);
							targetNodeID = candidateNodeID;
							break;
						}
					}
				}

				if (targetState != -1)
				{
					AnimationTransition& transition = target_->transitions_.emplace_back();
					transition.fromState_ = selectedSourceIndex_;
					transition.toState_ = targetState;
					transition.fromOffsetX_ = from.x - sourcePosition.x;
					transition.fromOffsetY_ = from.y - sourcePosition.y;

					Vector2 targetPosition = canvas_.NodePosition(targetNodeID);
					transition.toOffsetX_ = mouse.x - targetPosition.x;
					transition.toOffsetY_ = mouse.y - targetPosition.y;
				}

				selectedSourceIndex_ = -1;
			}
			else
			{
				canvas_.Guide(NodePin{ sourceNodeID, 0 }, mouse, Color(1.0f, 0.82f, 0.47f, 0.9f), from - sourcePosition);
			}
		}

		if (canvas_.BeginMenu(NodeTarget::Background))
		{
			if (ImGui::MenuItem("ステートを追加"))
			{
				AnimationState& state = target_->states_.emplace_back();
				state.name_ = String("State");
				state.nodePositionX_ = canvas_.MenuPosition().x;
				state.nodePositionY_ = canvas_.MenuPosition().y;
				canvas_.PlaceNode(static_cast<Uint32>(target_->states_.size()), canvas_.MenuPosition());
			}
			canvas_.EndMenu();
		}

		Uint32 hoveredNodeID = canvas_.HoveredNode();
		Bool menuless = (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) && (hoveredNodeID == entryNodeID_ || hoveredNodeID == exitNodeID_);
		if (!menuless && canvas_.BeginMenu(NodeTarget::Node))
		{
			Uint32 menuNodeID = canvas_.MenuNode();
			Int source = -1;
			if (menuNodeID == anyNodeID_)
			{
				source = Animator::AnyState;
			}
			else if (menuNodeID >= 1 && menuNodeID <= target_->states_.size())
			{
				source = static_cast<Int>(menuNodeID - 1);
			}

			if (source == -1)
			{
				ImGui::CloseCurrentPopup();
			}
			else
			{
				if (ImGui::MenuItem("遷移を作成"))
				{
					selectedSourceIndex_ = source;
				}

				if (source >= 0)
				{
					if (ImGui::MenuItem("デフォルトステートに設定"))
					{
						target_->entryStateIndex_ = source;
					}

					ImGui::Separator();

					if (ImGui::MenuItem("削除"))
					{
						removeState(static_cast<Size>(source));
					}
				}
			}
			canvas_.EndMenu();
		}

		canvas_.End();

		if (!removed)
		{
			for (Size index = 0; index < target_->states_.size(); ++index)
			{
				Vector2 position = canvas_.NodePosition(static_cast<Uint32>(index + 1));
				target_->states_[index].nodePositionX_ = position.x;
				target_->states_[index].nodePositionY_ = position.y;
			}
		}

		Vector2 entryPosition = canvas_.NodePosition(entryNodeID_);
		target_->entryNodePositionX_ = entryPosition.x;
		target_->entryNodePositionY_ = entryPosition.y;

		Vector2 exitPosition = canvas_.NodePosition(exitNodeID_);
		target_->exitNodePositionX_ = exitPosition.x;
		target_->exitNodePositionY_ = exitPosition.y;

		Vector2 anyPosition = canvas_.NodePosition(anyNodeID_);
		target_->anyNodePositionX_ = anyPosition.x;
		target_->anyNodePositionY_ = anyPosition.y;

		if (canvas_.Reselected())
		{
			selectedConditionIndex_ = -1;
		}
	}

	const Char* AnimatorControllerPanel::ComparisonLabel(AnimationConditionComparison comparison)
	{
		switch (comparison)
		{
		case AnimationConditionComparison::Equal:
			return "=";
		case AnimationConditionComparison::NotEqual:
			return "!=";
		case AnimationConditionComparison::Greater:
			return ">";
		case AnimationConditionComparison::Less:
			return "<";
		case AnimationConditionComparison::GreaterOrEqual:
			return ">=";
		case AnimationConditionComparison::LessOrEqual:
			return "<=";
		default:
			return "Unknown";
		}
	}

	const Char* AnimatorControllerPanel::ParameterLabel(AnimationParameterType type)
	{
		switch (type)
		{
		case AnimationParameterType::Bool:
			return "Bool";
		case AnimationParameterType::Float:
			return "Float";
		case AnimationParameterType::Int:
			return "Int";
		case AnimationParameterType::Trigger:
			return "Trigger";
		default:
			return "Unknown";
		}
	}

	const Char* AnimatorControllerPanel::AssetLabel(Int assetID)const
	{
		if (assetID == 0)
		{
			return "(未選択)";
		}

		AssetRecord* asset = context_.world_.resource_->Record(static_cast<Uint32>(assetID));
		if (!asset)
		{
			return "";
		}

		const Char* path = asset->path_.c_str();
		Size separator = std::string_view(path).find_last_of("/\\");
		return separator == std::string_view::npos ? path : path + separator + 1;
	}
}
