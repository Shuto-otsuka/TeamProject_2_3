#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

#include <External/SDL3/Include/SDL3/SDL.h>
#include <External/SDL3/Include/SDL3/SDL_gamepad.h>

#pragma comment(lib, "../External/SDL3/Lib/SDL3.lib")

namespace SeedCore
{
	namespace
	{
		struct AxisKeyRecord
		{
			Int32 up_ = 0;
			Int32 down_ = 0;
			Int32 left_ = 0;
			Int32 right_ = 0;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.Field("up", up_);
				archive.Field("down", down_);
				archive.Field("left", left_);
				archive.Field("right", right_);
			}
		};

		struct ActionBindingRecord
		{
			String action_;
			DynamicArray<Int32> keyList_;
			DynamicArray<Int32> gamepadList_;
			DynamicArray<AxisKeyRecord> axisKeyList_;
			DynamicArray<Int32> stickList_;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.Field("action", action_);
				archive.Field("keyList", keyList_);
				archive.Field("gamepadList", gamepadList_);
				archive.Field("axisKeyList", axisKeyList_);
				archive.Field("stickList", stickList_);
			}
		};
	}

	Bool InputSystem::gameInput_ = true;
	Bool InputSystem::currentKeyState_[KEY_COUNT] = {};
	Bool InputSystem::previousKeyState_[KEY_COUNT] = {};
	Bool InputSystem::currentMouseState_[MOUSE_BUTTON_COUNT] = {};
	Bool InputSystem::previousMouseState_[MOUSE_BUTTON_COUNT] = {};
	Vector2 InputSystem::gameMousePoint_ = Vector2(0.0f, 0.0f);
	Vector2 InputSystem::desktopMousePoint_ = Vector2(0.0f, 0.0f);
	Vector2 InputSystem::mouseMotion_ = Vector2(0.0f, 0.0f);
	Float InputSystem::currentMouseWheel_ = 0.0f;
	Float InputSystem::pendingMouseWheel_ = 0.0f;
	Bool InputSystem::mouseCaptured_ = false;
	Vector2 InputSystem::mouseCaptureAnchor_ = Vector2(0.0f, 0.0f);
	Vector2 InputSystem::mouseCaptureReturn_ = Vector2(0.0f, 0.0f);
	Bool InputSystem::cursorLocked_ = false;
	Vector2 InputSystem::cursorLockPoint_ = Vector2(0.0f, 0.0f);
	Bool InputSystem::cursorLockPointFixed_ = false;
	Bool InputSystem::cursorLockAnchored_ = false;
	Bool InputSystem::cursorHidden_ = false;
	Bool InputSystem::currentGamepadState_[GAMEPAD_BUTTON_COUNT] = {};
	Bool InputSystem::previousGamepadState_[GAMEPAD_BUTTON_COUNT] = {};
	Vector2 InputSystem::gamepadStickAxis_[2] = { Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f) };
	Float InputSystem::gamepadTriggerAxis_[2] = {};
	void* InputSystem::gamepad_ = nullptr;
	FlatMap<String, InputSystem::ActionBinding> InputSystem::actionBindings_;
	DynamicArray<String> InputSystem::actionNameList_;

	void InputSystem::Initialize()
	{
		SDL_Init(SDL_INIT_GAMEPAD);
		Load();
	}

	void InputSystem::Update(const Vector4& gameRect)
	{
		Bool active = GetActiveWindow() != nullptr;

		currentMouseWheel_ = active ? pendingMouseWheel_ : 0.0f;
		pendingMouseWheel_ = 0.0f;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_GAMEPAD_ADDED:
				if (!gamepad_)
				{
					gamepad_ = SDL_OpenGamepad(event.gdevice.which);
				}
				break;
			case SDL_EVENT_GAMEPAD_REMOVED:
				if (gamepad_)
				{
					SDL_CloseGamepad(static_cast<SDL_Gamepad*>(gamepad_));
					gamepad_ = nullptr;
				}
				break;
			}
		}

		BYTE keyboardState[256];
		if (!active || !GetKeyboardState(keyboardState))
		{
			memset(keyboardState, 0, sizeof(keyboardState));
		}

		for (Int index = 0; index < KEY_COUNT; ++index)
		{
			previousKeyState_[index] = currentKeyState_[index];
			currentKeyState_[index] = (keyboardState[index] & 0x80) != 0;
		}

		POINT point;
		GetCursorPos(&point);
		Vector2 cursor = Vector2(static_cast<Float>(point.x), static_cast<Float>(point.y));
		mouseMotion_ = Vector2(cursor.x - desktopMousePoint_.x, cursor.y - desktopMousePoint_.y);
		desktopMousePoint_ = cursor;

		Bool validGameRect = gameRect.z > 0.0f && gameRect.w > 0.0f;
		gameInput_ = active && validGameRect && cursor.x >= gameRect.x && cursor.x < gameRect.x + gameRect.z && cursor.y >= gameRect.y && cursor.y < gameRect.y + gameRect.w;

		if (validGameRect)
		{
			gameMousePoint_.x = (cursor.x - gameRect.x) / gameRect.z * 1920.0f;
			gameMousePoint_.y = (cursor.y - gameRect.y) / gameRect.w * 1080.0f;
		}

		if (!active)
		{
			mouseMotion_ = Vector2(0.0f, 0.0f);
		}

		if (mouseCaptured_ && active)
		{
			SetCursorPos(static_cast<Int>(mouseCaptureAnchor_.x), static_cast<Int>(mouseCaptureAnchor_.y));
			desktopMousePoint_ = mouseCaptureAnchor_;
		}

		if (cursorLocked_ && active && gameInput_ && !mouseCaptured_)
		{
			if (!cursorLockAnchored_)
			{
				if (!cursorLockPointFixed_)
				{
					cursorLockPoint_ = gameMousePoint_;
				}
				cursorLockAnchored_ = true;
			}

			Int desktopX = static_cast<Int>(gameRect.x + cursorLockPoint_.x / 1920.0f * gameRect.z);
			Int desktopY = static_cast<Int>(gameRect.y + cursorLockPoint_.y / 1080.0f * gameRect.w);
			SetCursorPos(desktopX, desktopY);
			desktopMousePoint_ = Vector2(static_cast<Float>(desktopX), static_cast<Float>(desktopY));
			gameMousePoint_ = cursorLockPoint_;
		}
		else
		{
			cursorLockAnchored_ = false;
		}

		SC_CONSTEX Int mouseVKeys[MOUSE_BUTTON_COUNT] =
		{
			VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2
		};
		for (Int index = 0; index < MOUSE_BUTTON_COUNT; ++index)
		{
			previousMouseState_[index] = currentMouseState_[index];
			currentMouseState_[index] = (keyboardState[mouseVKeys[index]] & 0x80) != 0;
		}

		if (gamepad_)
		{
			for (Int index = 0; index < GAMEPAD_BUTTON_COUNT; ++index)
			{
				previousGamepadState_[index] = currentGamepadState_[index];
				currentGamepadState_[index] = active && SDL_GetGamepadButton(static_cast<SDL_Gamepad*>(gamepad_), static_cast<SDL_GamepadButton>(index));
			}
			Vector2& leftStick = gamepadStickAxis_[static_cast<Int>(GamepadStick::Left)];
			Vector2& rightStick = gamepadStickAxis_[static_cast<Int>(GamepadStick::Right)];
			leftStick = active ? Vector2(Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_LEFTX)) / 32768.0f, -1.0f, 1.0f), -Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_LEFTY)) / 32768.0f, -1.0f, 1.0f)) : Vector2(0.0f, 0.0f);
			rightStick = active ? Vector2(Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_RIGHTX)) / 32768.0f, -1.0f, 1.0f), -Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_RIGHTY)) / 32768.0f, -1.0f, 1.0f)) : Vector2(0.0f, 0.0f);

			gamepadTriggerAxis_[static_cast<Int>(GamepadTrigger::Left)] = active ? Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_LEFT_TRIGGER)) / 32768.0f, 0.0f, 1.0f) : 0.0f;
			gamepadTriggerAxis_[static_cast<Int>(GamepadTrigger::Right)] = active ? Clamp(static_cast<Float>(SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(gamepad_), SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) / 32768.0f, 0.0f, 1.0f) : 0.0f;
		}
	}

	void InputSystem::Finalize()
	{
		if (gamepad_)
		{
			SDL_CloseGamepad(static_cast<SDL_Gamepad*>(gamepad_));
			gamepad_ = nullptr;
		}
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	}

	Bool InputSystem::KeyState(Key key, TriggerMode mode)
	{
		Int vkey = static_cast<Int>(key);
		if (vkey < 0 || vkey >= KEY_COUNT)
		{
			return false;
		}

		switch (mode)
		{
		case TriggerMode::RISING_EDGE:
			return !previousKeyState_[vkey] && currentKeyState_[vkey];
		case TriggerMode::FALLING_EDGE:
			return previousKeyState_[vkey] && !currentKeyState_[vkey];
		case TriggerMode::NONE:
			return currentKeyState_[vkey];
		}
		return false;
	}

	Bool InputSystem::MouseState(MouseButton button, TriggerMode mode)
	{
		Int index = static_cast<Int>(button);
		if (index < 0 || index >= MOUSE_BUTTON_COUNT)
		{
			return false;
		}

		switch (mode)
		{
		case TriggerMode::RISING_EDGE:
			return !previousMouseState_[index] && currentMouseState_[index];
		case TriggerMode::FALLING_EDGE:
			return previousMouseState_[index] && !currentMouseState_[index];
		case TriggerMode::NONE:
			return currentMouseState_[index];
		}
		return false;
	}

	Vector2 InputSystem::MouseMotion()
	{
		return mouseMotion_;
	}

	Float InputSystem::MouseWheel()
	{
		return currentMouseWheel_;
	}

	void InputSystem::MouseWheel(Float delta)
	{
		pendingMouseWheel_ += delta;
	}

	void InputSystem::BeginMouseCapture()
	{
		if (mouseCaptured_)
		{
			return;
		}

		mouseCaptured_ = true;

		POINT origin;
		GetCursorPos(&origin);
		mouseCaptureReturn_ = Vector2(static_cast<Float>(origin.x), static_cast<Float>(origin.y));
		mouseCaptureAnchor_ = mouseCaptureReturn_;

		ShowCursor(FALSE);
	}

	void InputSystem::EndMouseCapture()
	{
		if (!mouseCaptured_)
		{
			return;
		}

		mouseCaptured_ = false;

		ShowCursor(TRUE);
		SetCursorPos(static_cast<Int>(mouseCaptureReturn_.x), static_cast<Int>(mouseCaptureReturn_.y));
		desktopMousePoint_ = mouseCaptureReturn_;
	}

	void InputSystem::LockCursor()
	{
		if (cursorLocked_)
		{
			return;
		}

		cursorLocked_ = true;
		cursorLockPointFixed_ = false;
	}

	void InputSystem::LockCursor(Vector2 point)
	{
		cursorLocked_ = true;
		cursorLockPoint_ = point;
		cursorLockPointFixed_ = true;
	}

	void InputSystem::UnlockCursor()
	{
		cursorLocked_ = false;
		cursorLockPointFixed_ = false;
		cursorLockAnchored_ = false;
	}

	void InputSystem::HideCursor()
	{
		if (cursorHidden_)
		{
			return;
		}

		cursorHidden_ = true;
		ShowCursor(FALSE);
	}

	void InputSystem::RevealCursor()
	{
		if (!cursorHidden_)
		{
			return;
		}

		cursorHidden_ = false;
		ShowCursor(TRUE);
	}

	void InputSystem::RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs)
	{
		if (gamepad_)
		{
			SDL_RumbleGamepad(static_cast<SDL_Gamepad*>(gamepad_), lowFrequency, highFrequency, durationMs);
		}
	}

	void InputSystem::RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs)
	{
		if (gamepad_)
		{
			SDL_RumbleGamepadTriggers(static_cast<SDL_Gamepad*>(gamepad_), left, right, durationMs);
		}
	}

	void InputSystem::RegisterAction(String action)
	{
		if (actionBindings_.find(action) != actionBindings_.end())
		{
			return;
		}

		actionNameList_.push_back(action);
		actionBindings_[action] = ActionBinding();
	}

	void InputSystem::UnregisterAction(String action)
	{
		if (actionBindings_.erase(action))
		{
			SeedCore::erase(actionNameList_, action);
		}
	}

	void InputSystem::BindKey(String action, Key key)
	{
		if (actionBindings_.find(action) == actionBindings_.end())
		{
			actionNameList_.push_back(action);
		}
		ActionBinding& binding = actionBindings_[action];
		if (!std::ranges::contains(binding.keyList_, key))
		{
			binding.keyList_.push_back(key);
		}
	}

	void InputSystem::UnbindKey(String action, Key key)
	{
		auto iterator = actionBindings_.find(action);
		if (iterator == actionBindings_.end())
		{
			return;
		}

		DynamicArray<Key>& keyList = iterator->second.keyList_;
		SeedCore::erase(keyList, key);
	}

	const DynamicArray<InputSystem::Key>& InputSystem::BoundKey(String action)
	{
		static const DynamicArray<Key> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.keyList_ : empty;
	}

	void InputSystem::BindGamepad(String action, ScGamepadButton button)
	{
		if (actionBindings_.find(action) == actionBindings_.end())
		{
			actionNameList_.push_back(action);
		}
		ActionBinding& binding = actionBindings_[action];
		if (!std::ranges::contains(binding.gamepadList_, button))
		{
			binding.gamepadList_.push_back(button);
		}
	}

	void InputSystem::UnbindGamepad(String action, ScGamepadButton button)
	{
		auto iterator = actionBindings_.find(action);
		if (iterator == actionBindings_.end())
		{
			return;
		}

		DynamicArray<ScGamepadButton>& gamepadList = iterator->second.gamepadList_;
		SeedCore::erase(gamepadList, button);
	}

	const DynamicArray<InputSystem::ScGamepadButton>& InputSystem::BoundGamepad(String action)
	{
		static const DynamicArray<ScGamepadButton> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.gamepadList_ : empty;
	}

	void InputSystem::BindAxisKey(String action, DirectionalKey axisKey)
	{
		if (actionBindings_.find(action) == actionBindings_.end())
		{
			actionNameList_.push_back(action);
		}
		ActionBinding& binding = actionBindings_[action];
		if (!std::ranges::contains(binding.axisKeyList_, axisKey))
		{
			binding.axisKeyList_.push_back(axisKey);
		}
	}

	void InputSystem::UnbindAxisKey(String action, DirectionalKey axisKey)
	{
		auto iterator = actionBindings_.find(action);
		if (iterator == actionBindings_.end())
		{
			return;
		}

		DynamicArray<DirectionalKey>& axisKeyList = iterator->second.axisKeyList_;
		SeedCore::erase(axisKeyList, axisKey);
	}

	const DynamicArray<InputSystem::DirectionalKey>& InputSystem::BoundAxisKey(String action)
	{
		static const DynamicArray<DirectionalKey> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.axisKeyList_ : empty;
	}

	void InputSystem::BindStick(String action, GamepadStick stick)
	{
		if (actionBindings_.find(action) == actionBindings_.end())
		{
			actionNameList_.push_back(action);
		}
		ActionBinding& binding = actionBindings_[action];
		if (!std::ranges::contains(binding.stickList_, stick))
		{
			binding.stickList_.push_back(stick);
		}
	}

	void InputSystem::UnbindStick(String action, GamepadStick stick)
	{
		auto iterator = actionBindings_.find(action);
		if (iterator == actionBindings_.end())
		{
			return;
		}

		DynamicArray<GamepadStick>& stickList = iterator->second.stickList_;
		SeedCore::erase(stickList, stick);
	}

	const DynamicArray<InputSystem::GamepadStick>& InputSystem::BoundStick(String action)
	{
		static const DynamicArray<GamepadStick> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.stickList_ : empty;
	}

	const DynamicArray<String>& InputSystem::ActionNameList()
	{
		return actionNameList_;
	}

	void InputSystem::Load(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			return;
		}

		BinaryInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			return;
		}

		DynamicArray<ActionBindingRecord> recordList;
		archive.TryField("actionBindingList", recordList);

		actionBindings_.clear();
		actionNameList_.clear();

		for (const ActionBindingRecord& record : recordList)
		{
			if (actionBindings_.find(record.action_) == actionBindings_.end())
			{
				actionNameList_.push_back(record.action_);
			}
			ActionBinding& binding = actionBindings_[record.action_];

			std::ranges::transform(record.keyList_, std::back_inserter(binding.keyList_), [](Int32 key) { return static_cast<Key>(key); });
			std::ranges::transform(record.gamepadList_, std::back_inserter(binding.gamepadList_), [](Int32 button) { return static_cast<ScGamepadButton>(button); });

			for (const AxisKeyRecord& axisKeyRecord : record.axisKeyList_)
			{
				DirectionalKey axisKey;
				axisKey.up_ = static_cast<Key>(axisKeyRecord.up_);
				axisKey.down_ = static_cast<Key>(axisKeyRecord.down_);
				axisKey.left_ = static_cast<Key>(axisKeyRecord.left_);
				axisKey.right_ = static_cast<Key>(axisKeyRecord.right_);
				binding.axisKeyList_.push_back(axisKey);
			}

			std::ranges::transform(record.stickList_, std::back_inserter(binding.stickList_), [](Int32 stick) { return static_cast<GamepadStick>(stick); });
		}
	}

	void InputSystem::Save(const std::filesystem::path& path)
	{
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		DynamicArray<ActionBindingRecord> recordList;
		recordList.reserve(actionNameList_.size());

		for (const String& action : actionNameList_)
		{
			const ActionBinding& binding = actionBindings_.at(action);

			ActionBindingRecord record;
			record.action_ = action;

			std::ranges::transform(binding.keyList_, std::back_inserter(record.keyList_), [](Key key) { return static_cast<Int32>(key); });
			std::ranges::transform(binding.gamepadList_, std::back_inserter(record.gamepadList_), [](ScGamepadButton button) { return static_cast<Int32>(button); });

			for (const DirectionalKey& axisKey : binding.axisKeyList_)
			{
				AxisKeyRecord axisKeyRecord;
				axisKeyRecord.up_ = static_cast<Int32>(axisKey.up_);
				axisKeyRecord.down_ = static_cast<Int32>(axisKey.down_);
				axisKeyRecord.left_ = static_cast<Int32>(axisKey.left_);
				axisKeyRecord.right_ = static_cast<Int32>(axisKey.right_);
				record.axisKeyList_.push_back(axisKeyRecord);
			}

			std::ranges::transform(binding.stickList_, std::back_inserter(record.stickList_), [](GamepadStick stick) { return static_cast<Int32>(stick); });

			recordList.push_back(std::move(record));
		}

		BinaryOutputArchive archive;
		archive.Field("actionBindingList", recordList);
		archive.Write(String(path.string()));
	}
}
