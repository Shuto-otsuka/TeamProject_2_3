#include <FoundationEngine/Input/Input.h>

namespace SeedCore
{
	Bool Input::KeyState(Key key, TriggerMode mode)
	{
		if (!InputSystem::gameInput_)
		{
			return false;
		}

		return InputSystem::KeyState(key, mode);
	}

	Bool Input::MouseState(MouseButton button, TriggerMode mode)
	{
		if (!InputSystem::gameInput_)
		{
			return false;
		}

		return InputSystem::MouseState(button, mode);
	}

	Bool Input::GamepadState(ScGamepadButton button, TriggerMode mode)
	{
		Int index = static_cast<Int>(button);
		if (index < 0 || index >= InputSystem::GAMEPAD_BUTTON_COUNT)
		{
			return false;
		}

		switch (mode)
		{
		case TriggerMode::RISING_EDGE:
			return !InputSystem::previousGamepadState_[index] && InputSystem::currentGamepadState_[index];
		case TriggerMode::FALLING_EDGE:
			return InputSystem::previousGamepadState_[index] && !InputSystem::currentGamepadState_[index];
		case TriggerMode::NONE:
			return InputSystem::currentGamepadState_[index];
		}
		return false;
	}

	Bool Input::ActionState(String action, TriggerMode mode)
	{
		auto iterator = InputSystem::actionBindings_.find(action);
		if (iterator == InputSystem::actionBindings_.end())
		{
			return false;
		}

		return std::ranges::any_of(iterator->second.keyList_, [&](Key key) { return KeyState(key, mode); }) || std::ranges::any_of(iterator->second.gamepadList_, [&](ScGamepadButton button) { return GamepadState(button, mode); });
	}

	Vector2 Input::MousePoint()
	{
		return InputSystem::gameMousePoint_;
	}

	Vector2 Input::MouseMotion()
	{
		if (!InputSystem::gameInput_)
		{
			return Vector2(0.0f, 0.0f);
		}

		return InputSystem::MouseMotion();
	}

	Float Input::MouseWheel()
	{
		if (!InputSystem::gameInput_)
		{
			return 0.0f;
		}

		return InputSystem::MouseWheel();
	}

	Vector2 Input::GamepadAxis(GamepadStick stick)
	{
		return InputSystem::gamepadStickAxis_[static_cast<Int>(stick)];
	}

	Float Input::GamepadAxis(GamepadTrigger trigger)
	{
		return InputSystem::gamepadTriggerAxis_[static_cast<Int>(trigger)];
	}

	Vector2 Input::ActionAxis(String action)
	{
		auto iterator = InputSystem::actionBindings_.find(action);
		if (iterator == InputSystem::actionBindings_.end())
		{
			return Vector2(0.0f, 0.0f);
		}

		Vector2 keyboardVector(0.0f, 0.0f);
		for (const DirectionalKey& axisKey : iterator->second.axisKeyList_)
		{
			if (KeyState(axisKey.up_, TriggerMode::NONE))
			{
				keyboardVector.y += 1.0f;
			}
			if (KeyState(axisKey.down_, TriggerMode::NONE))
			{
				keyboardVector.y -= 1.0f;
			}
			if (KeyState(axisKey.left_, TriggerMode::NONE))
			{
				keyboardVector.x -= 1.0f;
			}
			if (KeyState(axisKey.right_, TriggerMode::NONE))
			{
				keyboardVector.x += 1.0f;
			}
		}

		if (keyboardVector.x != 0.0f || keyboardVector.y != 0.0f)
		{
			keyboardVector.Normalize();
			return keyboardVector;
		}

		for (GamepadStick stick : iterator->second.stickList_)
		{
			Vector2 tilt = GamepadAxis(stick);
			if (tilt.x != 0.0f || tilt.y != 0.0f)
			{
				return tilt;
			}
		}

		return Vector2(0.0f, 0.0f);
	}

	void Input::RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs)
	{
		InputSystem::RumbleBody(lowFrequency, highFrequency, durationMs);
	}

	void Input::RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs)
	{
		InputSystem::RumbleTrigger(left, right, durationMs);
	}

	void Input::LockCursor()
	{
		InputSystem::LockCursor();
	}

	void Input::LockCursor(Vector2 point)
	{
		InputSystem::LockCursor(point);
	}

	void Input::UnlockCursor()
	{
		InputSystem::UnlockCursor();
	}

	void Input::HideCursor()
	{
		InputSystem::HideCursor();
	}

	void Input::RevealCursor()
	{
		InputSystem::RevealCursor();
	}
}
