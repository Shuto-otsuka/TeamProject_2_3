#pragma once
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class Actor;
	class CompoundCommand;

	class HierarchyPanel
	{
	public:
		HierarchyPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~HierarchyPanel() = default;

		void Draw();

	private:
		void DrawActorNode(Actor actor);

		void SaveAsPrefab(Actor actor);

		String GetUniqueName();

		void HandleNodeSelection(Actor actor, Bool ctrl, Bool shift);

		Bool Selected(Actor actor)const;

		Uint32 PrevSiblingPersistentId(Actor actor)const;

	private:
		struct RowRect
		{
			Actor actor_;
			ImVec2 min_;
			ImVec2 max_;
		};

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		DynamicArray<RowRect> rows_;

		Actor pendingClickActor_;
		Bool pendingClickCtrl_ = false;
		Bool pendingClickShift_ = false;

		Actor rangeAnchor_;

		Bool marqueeActive_ = false;
		ImVec2 marqueeStart_ = ImVec2(0.0f, 0.0f);
		Float marqueeScrollY_ = 0.0f;
	};
}
