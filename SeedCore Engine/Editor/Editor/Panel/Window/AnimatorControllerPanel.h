#pragma once
#include <Editor/Editor/Node/NodeCanvas.h>

namespace SeedCore
{
	struct EditorContext;
	class Animator;
	class ImGuiTexture;
	enum class AnimationConditionComparison;
	enum class AnimationParameterType;

	class AnimatorControllerPanel
	{
	public:
		AnimatorControllerPanel(EditorContext& context, ImGuiTexture& imguiTexture);

		~AnimatorControllerPanel() = default;

		void Draw();

		void Open(Animator* target);

		void Inspect();

		NODISCARD Bool Focused()const;

	private:
		void DrawNodeEditor();

	private:
		NODISCARD static const Char* ComparisonLabel(AnimationConditionComparison comparison);

		NODISCARD static const Char* ParameterLabel(AnimationParameterType type);

		NODISCARD const Char* AssetLabel(Int assetID)const;

	private:
		SC_CONSTEX Uint32 entryNodeID_ = 900001;

		SC_CONSTEX Uint32 exitNodeID_ = 900002;

		SC_CONSTEX Uint32 anyNodeID_ = 900003;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		NodeCanvas canvas_;

		Animator* target_ = nullptr;

		Int selectedSourceIndex_ = -1;

		Int selectedConditionIndex_ = -1;

		Bool altActive_ = false;
	};
}
