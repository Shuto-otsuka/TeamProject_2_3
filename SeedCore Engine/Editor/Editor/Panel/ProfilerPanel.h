#pragma once

namespace SeedCore
{
	struct EditorContext;

	class ProfilerPanel
	{
	public:
		ProfilerPanel(EditorContext& context);
		~ProfilerPanel() = default;

		void Draw();

	private:
		EditorContext& context_;

		SC_CONSTEX Int historySize_ = 240;
		Float fpsHistory_[historySize_] = {};
		Float frameTimeHistory_[historySize_] = {};
		Int historyOffset_ = 0;

		Float displayFPS_ = 0.0f;
		Float displayFrameTime_ = 0.0f;
		Float refreshTimer_ = 0.0f;
		SC_CONSTEX Float refreshInterval_ = 0.5f;
	};
}
