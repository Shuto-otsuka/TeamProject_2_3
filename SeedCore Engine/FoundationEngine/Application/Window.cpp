#if SC_DEBUG
#include <crtdbg.h>
#endif

#include <FoundationEngine/Application/Window.h>
#include <FoundationEngine/Utility/Bootstrap.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>

namespace SeedCore
{
	Bool Window::Initialize(const Bootstrap& bootstrap, const GameConfig& gameConfig)
	{
		std::wstring title = gameConfig.executableName_.w_str();
		const Wchar* windowTitle = title.empty() ? bootstrap.WindowDesc_.Title_ : title.c_str();

		if (!Initialize(windowTitle, windowTitle, gameConfig.windowWidth_, gameConfig.windowHeight_))
		{
			return false;
		}

		fullscreenToggle_ = true;

		if (gameConfig.fullscreen_)
		{
			GetWindowRect(hwnd_, &windowedRect_);

			MONITORINFO monitorInfo{};
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);

			SetWindowLongPtr(hwnd_, GWL_STYLE, static_cast<LONG_PTR>(WS_POPUP | WS_VISIBLE));
			SetWindowPos(hwnd_, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

			fullscreen_ = true;
		}

		ShowWindow(hwnd_, SW_SHOW);
		UpdateWindow(hwnd_);

		timer_.Start();
		InputSystem::Initialize();
		return true;
	}

	Bool Window::Initialize(const Bootstrap& bootstrap, const EditorConfig& editorConfig)
	{
#if SC_DEBUG
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		if (!Initialize(bootstrap.WindowDesc_.Title_, L"SeedCore Engine", bootstrap.WindowDesc_.Width_, bootstrap.WindowDesc_.Height_))
		{
			return false;
		}

		ShowWindow(hwnd_, SW_MAXIMIZE);
		UpdateWindow(hwnd_);

		timer_.Start();
		InputSystem::Initialize();
		return true;
	}

	void Window::Handler(MessageHandler handler)
	{
		messageHandler_ = handler;
	}

	void Window::CalculateFrameStats()
	{
		if (++framesPerSecond_, (timer_.Total() - countBySeconds_) >= 1.0f)
		{
			fps_ = static_cast<Float>(framesPerSecond_);
			framesPerSecond_ = 0;
			countBySeconds_ += 1.0f;
		}
	}

	Bool Window::ConsumeClosing()
	{
		if (closeRequested_)
		{
			closeRequested_ = false;
			return true;
		}
		return false;
	}

	Bool Window::ConsumeResized(Uint32& width, Uint32& height)
	{
		if (!resized_)
		{
			return false;
		}

		resized_ = false;
		width = clientWidth_;
		height = clientHeight_;
		return true;
	}

	void Window::RestoreAccessibility()
	{
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &originalStickyKeys_, 0);
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &originalToggleKeys_, 0);
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &originalFilterKeys_, 0);
	}

	Timer& Window::GetTimer()
	{
		return timer_;
	}

	HWND Window::Handle()const
	{
		return hwnd_;
	}

	Bool Window::Initialize(const Wchar* className, const Wchar* caption, Uint32 width, Uint32 height)
	{
		instance_ = GetModuleHandle(nullptr);

		originalStickyKeys_.cbSize = sizeof(STICKYKEYS);
		originalToggleKeys_.cbSize = sizeof(TOGGLEKEYS);
		originalFilterKeys_.cbSize = sizeof(FILTERKEYS);
		SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &originalStickyKeys_, 0);
		SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &originalToggleKeys_, 0);
		SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &originalFilterKeys_, 0);

		STICKYKEYS stickyKeys{ sizeof(STICKYKEYS), 0 };
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &stickyKeys, 0);

		TOGGLEKEYS toggleKeys{ sizeof(TOGGLEKEYS), 0 };
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &toggleKeys, 0);

		FILTERKEYS filterKeys{ sizeof(FILTERKEYS), 0 };
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &filterKeys, 0);

		HICON icon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
		HICON smallIcon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
		if (!icon)
		{
			icon = static_cast<HICON>(LoadImageW(nullptr, L"../Runtime/Logo/SeedCore.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));
			smallIcon = static_cast<HICON>(LoadImageW(nullptr, L"../Runtime/Logo/SeedCore.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE));
		}

		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WindowProcedure;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = instance_;
		wcex.hIcon = icon;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = className;
		wcex.hIconSm = smallIcon;
		RegisterClassExW(&wcex);

		windowedStyle_ = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;

		RECT rect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		AdjustWindowRect(&rect, windowedStyle_, FALSE);
		hwnd_ = CreateWindowExW(0, className, caption, windowedStyle_, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, instance_, nullptr);
		if (!hwnd_)
		{
			return false;
		}

		SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		return true;
	}

	LRESULT CALLBACK Window::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return window ? window->HandleMessage(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
	}

	LRESULT CALLBACK Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (messageHandler_ && messageHandler_(hwnd, msg, wparam, lparam))
		{
			return true;
		}

		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
		}
		break;
		case WM_CLOSE:
		{
			closeRequested_ = true;
			return 0;
		}
		case WM_DESTROY:
		{
			RestoreAccessibility();
			PostQuitMessage(0);
		}
		break;
		case WM_ENTERSIZEMOVE:
		{
			timer_.Stop();
		}
		break;
		case WM_EXITSIZEMOVE:
		{
			timer_.Start();
		}
		break;
		case WM_SIZE:
		{
			Uint32 width = static_cast<Uint32>(LOWORD(lparam));
			Uint32 height = static_cast<Uint32>(HIWORD(lparam));
			if (wparam != SIZE_MINIMIZED && width > 0 && height > 0 && (width != clientWidth_ || height != clientHeight_))
			{
				clientWidth_ = width;
				clientHeight_ = height;
				resized_ = true;
			}
		}
		break;
		case WM_SYSKEYDOWN:
		{
			if (fullscreenToggle_ && wparam == VK_RETURN && (lparam & (1 << 29)))
			{
				if (!fullscreen_)
				{
					GetWindowRect(hwnd_, &windowedRect_);

					MONITORINFO monitorInfo{};
					monitorInfo.cbSize = sizeof(MONITORINFO);
					GetMonitorInfo(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);

					SetWindowLongPtr(hwnd_, GWL_STYLE, static_cast<LONG_PTR>(WS_POPUP | WS_VISIBLE));
					SetWindowPos(hwnd_, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

					fullscreen_ = true;
				}
				else
				{
					SetWindowLongPtr(hwnd_, GWL_STYLE, static_cast<LONG_PTR>(windowedStyle_ | WS_VISIBLE));
					SetWindowPos(hwnd_, HWND_NOTOPMOST, windowedRect_.left, windowedRect_.top, windowedRect_.right - windowedRect_.left, windowedRect_.bottom - windowedRect_.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

					fullscreen_ = false;
				}
				return 0;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case WM_MENUCHAR:
		{
			if (fullscreenToggle_)
			{
				return MAKELRESULT(0, MNC_CLOSE);
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		case WM_MOUSEWHEEL:
		{
			InputSystem::MouseWheel(static_cast<Float>(GET_WHEEL_DELTA_WPARAM(wparam)) / 120.0f);
		}
		break;
		default:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		}
		return 0;
	}
}
