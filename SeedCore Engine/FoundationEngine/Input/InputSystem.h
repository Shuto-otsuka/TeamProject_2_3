#pragma once
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class SEEDCORE_API InputSystem
	{
	public:
		enum class TriggerMode
		{
			NONE,

			RISING_EDGE,

			FALLING_EDGE
		};

		enum class Key : Int
		{
			Backspace = VK_BACK,
			Tab = VK_TAB,
			Enter = VK_RETURN,
			Shift = VK_SHIFT,
			Control = VK_CONTROL,
			Alt = VK_MENU,
			CapsLock = VK_CAPITAL,
			Escape = VK_ESCAPE,
			Space = VK_SPACE,
			PageUp = VK_PRIOR,
			PageDown = VK_NEXT,
			Home = VK_HOME,
			End = VK_END,
			Left = VK_LEFT,
			Up = VK_UP,
			Right = VK_RIGHT,
			Down = VK_DOWN,
			Delete = VK_DELETE,

			LeftShift = VK_LSHIFT,
			RightShift = VK_RSHIFT,
			LeftControl = VK_LCONTROL,
			RightControl = VK_RCONTROL,
			LeftAlt = VK_LMENU,
			RightAlt = VK_RMENU,

			Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
			Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

			A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G',
			H = 'H', I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N',
			O = 'O', P = 'P', Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U',
			V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',

			F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4,
			F5 = VK_F5, F6 = VK_F6, F7 = VK_F7, F8 = VK_F8,
			F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12
		};

		enum class MouseButton : Int
		{
			Left,
			Right,
			Middle,
			Extra1,
			Extra2
		};

		enum ScGamepadButton : Int
		{
			ScGamepadButtonInvalid = -1,
			ScGamepadButtonSouth,
			ScGamepadButtonEast,
			ScGamepadButtonWest,
			ScGamepadButtonNorth,
			ScGamepadButtonBack,
			ScGamepadButtonGuide,
			ScGamepadButtonStart,
			ScGamepadButtonLeftStick,
			ScGamepadButtonRightStick,
			ScGamepadButtonLeftShoulder,
			ScGamepadButtonRightShoulder,
			ScGamepadButtonDPadUp,
			ScGamepadButtonDPadDown,
			ScGamepadButtonDPadLeft,
			ScGamepadButtonDPadRight,
			ScGamepadButtonMisc1,
			ScGamepadButtonRightPaddle1,
			ScGamepadButtonLeftPaddle1,
			ScGamepadButtonRightPaddle2,
			ScGamepadButtonLeftPaddle2,
			ScGamepadButtonTouchpad,
			ScGamepadButtonMisc2,
			ScGamepadButtonMisc3,
			ScGamepadButtonMisc4,
			ScGamepadButtonMisc5,
			ScGamepadButtonMisc6,
			ScGamepadButtonCount
		};

		enum class GamepadStick
		{
			Left,
			Right
		};

		enum class GamepadTrigger
		{
			Left,
			Right
		};

		struct DirectionalKey
		{
			Key up_ = Key::Up;
			Key down_ = Key::Down;
			Key left_ = Key::Left;
			Key right_ = Key::Right;

			Bool operator==(const DirectionalKey& other)const
			{
				return up_ == other.up_ && down_ == other.down_ && left_ == other.left_ && right_ == other.right_;
			}
		};

	public:
		SC_CONSTEX TriggerMode IsPressed = TriggerMode::NONE;

		SC_CONSTEX TriggerMode OnPressed = TriggerMode::RISING_EDGE;

		SC_CONSTEX TriggerMode OnReleased = TriggerMode::FALLING_EDGE;

	public:
		static void Initialize();

		static void Update(const Vector4& gameRect);

		static void Finalize();

	public:
		static Bool KeyState(Key key, TriggerMode mode = TriggerMode::NONE);

		static Bool MouseState(MouseButton button, TriggerMode mode = TriggerMode::NONE);

		static Vector2 MouseMotion();

		static Float MouseWheel();

		static void MouseWheel(Float delta);

	public:
		static void BeginMouseCapture();

		static void EndMouseCapture();

	public:
		static void LockCursor();

		static void LockCursor(Vector2 point);

		static void UnlockCursor();

		static void HideCursor();

		static void RevealCursor();

	public:
		static void RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs);

		static void RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs);

	public:
		static void RegisterAction(String action);

		static void UnregisterAction(String action);

		static void BindKey(String action, Key key);

		static void UnbindKey(String action, Key key);

		static const DynamicArray<Key>& BoundKey(String action);

		static void BindGamepad(String action, ScGamepadButton button);

		static void UnbindGamepad(String action, ScGamepadButton button);

		static const DynamicArray<ScGamepadButton>& BoundGamepad(String action);

		static void BindAxisKey(String action, DirectionalKey axisKey);

		static void UnbindAxisKey(String action, DirectionalKey axisKey);

		static const DynamicArray<DirectionalKey>& BoundAxisKey(String action);

		static void BindStick(String action, GamepadStick stick);

		static void UnbindStick(String action, GamepadStick stick);

		static const DynamicArray<GamepadStick>& BoundStick(String action);

		static const DynamicArray<String>& ActionNameList();

		static void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/InputBindings.scg");

		static void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/InputBindings.scg");

	private:
		friend class Input;

		struct ActionBinding
		{
			DynamicArray<Key> keyList_;

			DynamicArray<ScGamepadButton> gamepadList_;

			DynamicArray<DirectionalKey> axisKeyList_;

			DynamicArray<GamepadStick> stickList_;
		};

		InputSystem() = delete;

	private:
		SC_CONSTEX Int KEY_COUNT = 256;

		SC_CONSTEX Int MOUSE_BUTTON_COUNT = 5;

		SC_CONSTEX Int GAMEPAD_BUTTON_COUNT = ScGamepadButtonCount;

		static Bool gameInput_;

		static Bool currentKeyState_[KEY_COUNT];

		static Bool previousKeyState_[KEY_COUNT];

		static Bool currentMouseState_[MOUSE_BUTTON_COUNT];

		static Bool previousMouseState_[MOUSE_BUTTON_COUNT];

		static Vector2 gameMousePoint_;

		static Vector2 desktopMousePoint_;

		static Vector2 mouseMotion_;

		static Float currentMouseWheel_;

		static Float pendingMouseWheel_;

		static Bool mouseCaptured_;

		static Vector2 mouseCaptureAnchor_;

		static Vector2 mouseCaptureReturn_;

		static Bool cursorLocked_;

		static Vector2 cursorLockPoint_;

		static Bool cursorLockPointFixed_;

		static Bool cursorLockAnchored_;

		static Bool cursorHidden_;

		static Bool currentGamepadState_[GAMEPAD_BUTTON_COUNT];

		static Bool previousGamepadState_[GAMEPAD_BUTTON_COUNT];

		static Vector2 gamepadStickAxis_[2];

		static Float gamepadTriggerAxis_[2];

		static void* gamepad_;

		static FlatMap<String, ActionBinding> actionBindings_;

		static DynamicArray<String> actionNameList_;
	};
}
