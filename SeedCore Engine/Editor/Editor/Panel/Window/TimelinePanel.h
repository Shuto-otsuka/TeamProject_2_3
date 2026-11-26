#pragma once

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class Animator;

	class TimelinePanel
	{
	public:
		TimelinePanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~TimelinePanel() = default;

		void Draw();

		void Open();

		void Inspect();

		NODISCARD Bool Focused()const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		Animator* target_ = nullptr;

		Size selectedAnimationIndex_ = SIZE_MAX;
		Float scrubTime_ = 0.0f;
		Bool playActive_ = false;

		Size selectedNotifyIndex_ = SIZE_MAX;
		Size selectedSpeedKeyIndex_ = SIZE_MAX;
		Bool keyframeDragActive_ = false;
		Bool speedGraphActive_ = false;
	};
}
