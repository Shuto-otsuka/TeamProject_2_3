#pragma once
#include <FoundationEngine/Input/InputSystem.h>

namespace SeedCore
{
	class SEEDCORE_API Input
	{
	public:
		using TriggerMode = InputSystem::TriggerMode;

		using Key = InputSystem::Key;

		using MouseButton = InputSystem::MouseButton;

		using ScGamepadButton = InputSystem::ScGamepadButton;

		using GamepadStick = InputSystem::GamepadStick;

		using GamepadTrigger = InputSystem::GamepadTrigger;

		using DirectionalKey = InputSystem::DirectionalKey;

		SC_CONSTEX TriggerMode IsPressed = TriggerMode::NONE;

		SC_CONSTEX TriggerMode OnPressed = TriggerMode::RISING_EDGE;

		SC_CONSTEX TriggerMode OnReleased = TriggerMode::FALLING_EDGE;

	public:
		static Bool KeyState(Key key, TriggerMode mode = TriggerMode::NONE);

		static Bool MouseState(MouseButton button, TriggerMode mode = TriggerMode::NONE);

		static Bool GamepadState(ScGamepadButton button, TriggerMode mode = TriggerMode::NONE);

		static Bool ActionState(String action, TriggerMode mode = TriggerMode::NONE);

		static Vector2 MousePoint();

		static Vector2 MouseMotion();

		static Float MouseWheel();

		static Vector2 GamepadAxis(GamepadStick stick);

		static Float GamepadAxis(GamepadTrigger trigger);

		static Vector2 ActionAxis(String action);

		static void RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs);

		static void RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs);

	public:
		static void LockCursor();

		static void LockCursor(Vector2 point);

		static void UnlockCursor();

		static void HideCursor();

		static void RevealCursor();

	private:
		Input() = delete;
	};
}
