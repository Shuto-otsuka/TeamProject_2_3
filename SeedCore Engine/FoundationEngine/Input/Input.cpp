#include <FoundationEngine/Input/Input.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns whether key satisfies mode. False on frames not handed to
	* the game.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* key が mode を満たすかを返す。ゲームに渡さないフレームでは false。
	*/
	Bool Input::KeyState(Key key, TriggerMode mode)
	{
		/// [EN] A frame not handed to the game reads as nothing held and no transition.
		/// [JP] ゲームに渡さないフレームは、何も押されておらず遷移も無いものとして読む。
		if (!InputSystem::gameInput_)
		{
			return false;
		}

		return InputSystem::KeyState(key, mode);
	}

	/**
	* [EN]
	* Returns whether mouse button button satisfies mode. False on frames
	* not handed to the game.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マウスボタン button が mode を満たすかを返す。ゲームに渡さない
	* フレームでは false。
	*/
	Bool Input::MouseState(MouseButton button, TriggerMode mode)
	{
		/// [EN] A frame not handed to the game reads as nothing held and no transition.
		/// [JP] ゲームに渡さないフレームは、何も押されておらず遷移も無いものとして読む。
		if (!InputSystem::gameInput_)
		{
			return false;
		}

		return InputSystem::MouseState(button, mode);
	}

	/**
	* [EN]
	* Returns whether gamepad button button satisfies mode. Returns false
	* if button is out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームパッドボタン button が mode を満たすかを返す。button が
	* 範囲外なら false。
	*/
	Bool Input::GamepadState(SDL_GamepadButton button, TriggerMode mode)
	{
		/// [EN] The gamepad is not tied to where the cursor is, so it is never filtered by gameInput_.
		/// [JP] ゲームパッドはカーソルの位置と関係ないので、gameInput_ では絞り込まない。
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

	/**
	* [EN]
	* Returns whether any key or gamepad button bound to action satisfies
	* mode. Keys follow KeyState(), buttons GamepadState().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action に紐づくいずれかのキーまたはゲームパッドボタンが mode を
	* 満たすかを返す。キーは KeyState()、ボタンは GamepadState() に従う。
	*/
	Bool Input::ActionState(String action, TriggerMode mode)
	{
		auto iterator = InputSystem::actionBindings_.find(action);
		if (iterator == InputSystem::actionBindings_.end())
		{
			return false;
		}

		/// [EN] Going through this class's KeyState keeps the keys filtered on frames not handed to the game, while the buttons still count.
		/// [JP] このクラスの KeyState を通すので、ゲームに渡さないフレームではキーは絞り込まれ、ボタンはそのまま数える。
		return std::ranges::any_of(iterator->second.keyList_, [&](Key key) { return KeyState(key, mode); }) || std::ranges::any_of(iterator->second.gamepadList_, [&](SDL_GamepadButton button) { return GamepadState(button, mode); });
	}

	/**
	* [EN]
	* Returns the cursor's current position, in screen pixels.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルの現在の位置を、画面ピクセル単位で返す。
	*/
	Vector2 Input::MousePoint()
	{
		/// [EN] Where the cursor is stays meaningful even on frames not handed to the game, so the position is never filtered.
		/// [JP] カーソルがどこにあるかはゲームに渡さないフレームでも意味があるので、位置は絞り込まない。
		return InputSystem::mousePoint_;
	}

	/**
	* [EN]
	* Returns the cursor's movement since last frame, in screen pixels.
	* Zero on frames not handed to the game.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前フレームからのカーソルの移動量を、画面ピクセル単位で返す。
	* ゲームに渡さないフレームでは 0。
	*/
	Vector2 Input::MouseMotion()
	{
		if (!InputSystem::gameInput_)
		{
			return Vector2(0.0f, 0.0f);
		}

		return InputSystem::MouseMotion();
	}

	/**
	* [EN]
	* Returns the mouse wheel rotation during last frame, in notches.
	* Zero on frames not handed to the game.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前フレーム中のマウスホイールの回転量を、ノッチ単位で返す。ゲームに
	* 渡さないフレームでは 0。
	*/
	Float Input::MouseWheel()
	{
		if (!InputSystem::gameInput_)
		{
			return 0.0f;
		}

		return InputSystem::MouseWheel();
	}

	/**
	* [EN]
	* Returns stick's tilt, each axis in [-1, 1], with up as +Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* stick の倒し具合を、各軸 [-1, 1] で返す。上が +Y。
	*/
	Vector2 Input::GamepadAxis(GamepadStick stick)
	{
		/// [EN] InputSystem already stores the tilt with up as +Y, matching ActionAxis() and the directional-key composites.
		/// [JP] InputSystem が上を +Y にして保存しているので、ActionAxis() や方向キー組と同じ向きのまま返せる。
		return InputSystem::gamepadStickAxis_[static_cast<Int>(stick)];
	}

	/**
	* [EN]
	* Returns how far trigger is pulled, from 0 (released) to 1 (fully
	* pulled).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* trigger の引き具合を、0（離している）から 1（引き切り）で返す。
	*/
	Float Input::GamepadAxis(GamepadTrigger trigger)
	{
		return InputSystem::gamepadTriggerAxis_[static_cast<Int>(trigger)];
	}

	/**
	* [EN]
	* Returns action's combined 2D input: the held directional-key
	* composites, normalized; if none are held, the first bound stick
	* reporting nonzero tilt; otherwise (0, 0). Up is +Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action の入力を合成した2次元の値を返す: 押されている方向キー組を
	* 正規化して返す。どれも押されていなければ、紐づくスティックのうち
	* 最初に非ゼロの傾きを報告したものを返す。どちらも無ければ (0, 0)。
	* 上が +Y。
	*/
	Vector2 Input::ActionAxis(String action)
	{
		auto iterator = InputSystem::actionBindings_.find(action);
		if (iterator == InputSystem::actionBindings_.end())
		{
			return Vector2(0.0f, 0.0f);
		}

		/// [EN] Every held composite adds its direction; going through this class's KeyState keeps them filtered on frames not handed to the game.
		/// [JP] 押されている方向キー組はそれぞれ向きを足し合わせる。このクラスの KeyState を通すので、ゲームに渡さないフレームでは絞り込まれる。
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

		/// [EN] Keys win over sticks; normalized so a diagonal is no faster than a straight line.
		/// [JP] キーをスティックより優先する。斜めが真っ直ぐより速くならないよう正規化する。
		if (keyboardVector.x != 0.0f || keyboardVector.y != 0.0f)
		{
			keyboardVector.Normalize();
			return keyboardVector;
		}

		/// [EN] With no key held, the first bound stick that is actually tilted is used as it is (partial tilt keeps its magnitude).
		/// [JP] キーが押されていなければ、紐づくスティックのうち実際に倒されている最初のものをそのまま使う（倒し具合の大きさも保つ）。
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

	/**
	* [EN]
	* Rumbles the gamepad body's low/high frequency motors for durationMs
	* milliseconds; all zero stops it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームパッド本体の低周波/高周波モーターを durationMs ミリ秒間振動
	* させる。全て 0 なら止める。
	*/
	void Input::RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs)
	{
		InputSystem::RumbleBody(lowFrequency, highFrequency, durationMs);
	}

	/**
	* [EN]
	* Rumbles the motors inside the left/right triggers for durationMs
	* milliseconds; all zero stops it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 左右トリガーの中のモーターを durationMs ミリ秒間振動させる。全て 0
	* なら止める。
	*/
	void Input::RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs)
	{
		InputSystem::RumbleTrigger(left, right, durationMs);
	}

	/**
	* [EN]
	* Locks the cursor where it is; see InputSystem::LockCursor().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルをその場に固定する。InputSystem::LockCursor() を参照。
	*/
	void Input::LockCursor()
	{
		InputSystem::LockCursor();
	}

	/**
	* [EN]
	* Locks the cursor at point (screen pixels); see
	* InputSystem::LockCursor(Vector2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルを point（画面ピクセル）に固定する。
	* InputSystem::LockCursor(Vector2) を参照。
	*/
	void Input::LockCursor(Vector2 point)
	{
		InputSystem::LockCursor(point);
	}

	/**
	* [EN]
	* Releases the cursor lock; see InputSystem::UnlockCursor().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルの固定を解く。InputSystem::UnlockCursor() を参照。
	*/
	void Input::UnlockCursor()
	{
		InputSystem::UnlockCursor();
	}

	/**
	* [EN]
	* Hides the cursor; see InputSystem::HideCursor().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルを見えなくする。InputSystem::HideCursor() を参照。
	*/
	void Input::HideCursor()
	{
		InputSystem::HideCursor();
	}

	/**
	* [EN]
	* Shows the cursor again; see InputSystem::RevealCursor().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルを再び表示する。InputSystem::RevealCursor() を参照。
	*/
	void Input::RevealCursor()
	{
		InputSystem::RevealCursor();
	}
}
