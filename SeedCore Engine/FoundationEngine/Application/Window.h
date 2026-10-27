#pragma once
#include <FoundationEngine/Time/Timer.h>

namespace SeedCore
{
	struct Bootstrap;
	struct GameConfig;
	struct EditorConfig;

	class SEEDCORE_API Window :public NonTransferable
	{
	public:
		using MessageHandler = LRESULT(*)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	public:
		Window() = default;
		~Window() = default;

		Bool Initialize(const Bootstrap& bootstrap, const GameConfig& gameConfig);

		Bool Initialize(const Bootstrap& bootstrap, const EditorConfig& editorConfig);

		void Handler(MessageHandler handler);

		void CalculateFrameStats();

		Bool ConsumeClosing();

		Bool ConsumeResized(Uint32& width, Uint32& height);

		void RestoreAccessibility();

	public:
		NODISCARD Timer& GetTimer();

		NODISCARD HWND Handle()const;

	private:
		Bool Initialize(const Wchar* className, const Wchar* caption, Uint32 width, Uint32 height);

		static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		LRESULT CALLBACK HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	private:
		HINSTANCE instance_ = nullptr;

		HWND hwnd_ = nullptr;

		MessageHandler messageHandler_ = nullptr;

		Timer timer_;

		Float fps_ = 0.0f;

		Uint32 framesPerSecond_ = 0;

		Float countBySeconds_ = 0.0f;

		STICKYKEYS originalStickyKeys_{};
		TOGGLEKEYS originalToggleKeys_{};
		FILTERKEYS originalFilterKeys_{};

		DWORD windowedStyle_ = 0;

		RECT windowedRect_{};

		Bool fullscreenToggle_ = false;

		Bool fullscreen_ = false;

		Bool closeRequested_ = false;

		Bool resized_ = false;

		Uint32 clientWidth_ = 0;

		Uint32 clientHeight_ = 0;
	};
}
