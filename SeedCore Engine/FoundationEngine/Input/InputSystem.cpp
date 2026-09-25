#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	namespace
	{
		/**
		* [EN]
		* On-disk form of one directional-key composite (see InputSystem::Load/Save).
		* Key is stored as Int32 because the archive has no support for raw enums.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 方向キー1組のディスク上の形(InputSystem::Load/Save 参照)。
		* アーカイブは生の enum を扱えないので、Key は Int32 で保存する。
		*/
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

		/**
		* [EN]
		* On-disk form of one action's bindings (see InputSystem::Load/Save).
		* Key, SDL_GamepadButton and GamepadStick are stored as Int32 because the archive has no support for raw enums.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1アクション分のバインドのディスク上の形(InputSystem::Load/Save 参照)。
		* アーカイブは生の enum を扱えないので、Key と SDL_GamepadButton と GamepadStick は Int32 で保存する。
		*/
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
	Vector2 InputSystem::mousePoint_ = Vector2(0.0f, 0.0f);
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
	SDL_Gamepad* InputSystem::gamepad_ = nullptr;
	FlatMap<String, InputSystem::ActionBinding> InputSystem::actionBindings_;
	DynamicArray<String> InputSystem::actionNameList_;

	/**
	* [EN]
	* Initializes the SDL gamepad subsystem and loads the action binding
	* table (see Load()). Must be called once before any other
	* input method.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SDLのゲームパッドサブシステムを初期化し、アクションバインド
	* テーブルを読み込む（Load() 参照）。他のどの入力の
	* メソッドよりも先に、一度だけ呼び出す必要がある。
	*/
	void InputSystem::Initialize()
	{
		SDL_Init(SDL_INIT_GAMEPAD);
		Load();
	}

	/**
	* [EN]
	* Polls keyboard/mouse/gamepad state for the current frame, rotating
	* the previous-frame snapshot forward, and records whether this
	* frame's keyboard/mouse input belongs to the game. While the
	* application is not the active one, everything is polled as released
	* and still.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在フレームのキーボード/マウス/ゲームパッド状態をポーリングし、
	* 前フレームのスナップショットを繰り越し、このフレームのキーボード/
	* マウス入力をゲームに渡すかを記録する。アプリがアクティブでない間は、
	* 全て押されていない・動いていないものとして取り込む。
	*/
	void InputSystem::Update(Bool gameInput)
	{
		gameInput_ = gameInput;

		/// [EN] GetActiveWindow() only returns a window of this thread, so nullptr means the user is working in another application; the key table and the cursor are system-wide and would otherwise keep reporting what happens there.
		/// [JP] GetActiveWindow() はこのスレッドのウィンドウしか返さないので、nullptr ならユーザーは別のアプリを操作している。キー表もカーソルもシステム全体のものなので、そうしないと向こうでの操作を拾い続けてしまう。
		Bool active = GetActiveWindow() != nullptr;

		/// [EN] The frame's accumulated wheel rotation becomes readable via MouseWheel(); start a fresh accumulator for the next frame.
		/// [JP] このフレームで累積されたホイールの回転量を MouseWheel() で読み出せるようにし、次フレーム用の累積器を新しく開始する。
		currentMouseWheel_ = active ? pendingMouseWheel_ : 0.0f;
		pendingMouseWheel_ = 0.0f;

		/// [EN] Drain SDL's event queue solely to detect gamepad hot-plug; keyboard/mouse/axis state is polled directly below instead of via events.
		/// [JP] SDL のイベントキューを、ゲームパッドのホットプラグ検出のためだけに消化する。キーボード/マウス/軸の状態はイベント経由ではなく、以下で直接ポーリングする。
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
					SDL_CloseGamepad(gamepad_);
					gamepad_ = nullptr;
				}
				break;
			}
		}

		/// [EN] Snapshot the full Win32 virtual-key table once per frame; an all-zero table (nothing pressed) stands in while inactive or if the query itself fails.
		/// [JP] Win32 の仮想キーテーブル全体を、フレームごとに1回スナップショットする。非アクティブの間やクエリ自体が失敗した場合は、全ゼロ（何も押されていない）の表で代える。
		BYTE keyboardState[256];
		if (!active || !GetKeyboardState(keyboardState))
		{
			memset(keyboardState, 0, sizeof(keyboardState));
		}

		/// [EN] Roll last frame's snapshot into previousKeyState_ before overwriting currentKeyState_, so KeyState's edge detection has something to compare against.
		/// [JP] currentKeyState_ を上書きする前に、前フレームのスナップショットを previousKeyState_ へ繰り越す。これにより KeyState のエッジ検出が比較対象を持てるようにする。
		for (Int index = 0; index < KEY_COUNT; ++index)
		{
			previousKeyState_[index] = currentKeyState_[index];
			currentKeyState_[index] = (keyboardState[index] & 0x80) != 0;
		}

		/// [EN] Compute this frame's mouse movement from the raw screen-space cursor position before mousePoint_ gets overwritten.
		/// [JP] mousePoint_ が上書きされる前に、生のスクリーン空間でのカーソル位置から、このフレームのマウスの移動量を計算する。
		POINT point;
		GetCursorPos(&point);
		Vector2 cursor = Vector2(static_cast<Float>(point.x), static_cast<Float>(point.y));
		mouseMotion_ = Vector2(cursor.x - mousePoint_.x, cursor.y - mousePoint_.y);
		mousePoint_ = cursor;

		/// [EN] While inactive the cursor is moving over another application: the position is still tracked, so there is no jump on return, but no movement is reported.
		/// [JP] 非アクティブの間、カーソルは別のアプリの上を動いている。戻ったときに飛ばないよう位置は追い続けるが、移動量は報告しない。
		if (!active)
		{
			mouseMotion_ = Vector2(0.0f, 0.0f);
		}

		/// [EN] While captured (see BeginMouseCapture) the cursor is re-anchored every frame so it never reaches a monitor edge, where GetCursorPos() clamps and the movement would drop to 0.
		///      Skipped while inactive, so the cursor is never pinned while the user works in another application.
		/// [JP] キャプチャ中(BeginMouseCapture 参照)は毎フレームカーソルを戻し、モニタ端に届かないようにする。端では GetCursorPos() が止まり、移動量が 0 になるため。
		///      非アクティブの間は行わず、別のアプリを操作しているユーザーのカーソルを固定しない。
		if (mouseCaptured_ && active)
		{
			SetCursorPos(static_cast<Int>(mouseCaptureAnchor_.x), static_cast<Int>(mouseCaptureAnchor_.y));
			mousePoint_ = mouseCaptureAnchor_;
		}

		/// [EN] The game's cursor lock (see LockCursor) only holds while the app is active, the input goes to the game (so in the editor the cursor is free once it leaves the game view), and no capture is using the cursor.
		/// [JP] ゲームのカーソルロック(LockCursor 参照)が効くのは、アプリがアクティブで、入力をゲームに渡していて(エディタではゲームビューを出たカーソルは自由になる)、キャプチャがカーソルを使っていない間だけ。
		if (cursorLocked_ && active && gameInput_ && !mouseCaptured_)
		{
			/// [EN] Without a point given to LockCursor(Vector2), the first frame the lock applies takes the cursor's current position, so the cursor is held where the player left it (inside the game view in the editor).
			/// [JP] LockCursor(Vector2) で位置が指定されていなければ、ロックが効く最初のフレームで今のカーソル位置を取るので、カーソルはプレイヤーが置いた場所(エディタではゲームビューの中)に留まる。
			if (!cursorLockAnchored_)
			{
				if (!cursorLockPointFixed_)
				{
					cursorLockPoint_ = mousePoint_;
				}
				cursorLockAnchored_ = true;
			}

			/// [EN] Put back every frame, like capture: this frame's movement was already measured above from the previous lock point.
			/// [JP] キャプチャと同じく毎フレーム戻す。このフレームの移動量は、上で前回の固定位置から既に測ってある。
			SetCursorPos(static_cast<Int>(cursorLockPoint_.x), static_cast<Int>(cursorLockPoint_.y));
			mousePoint_ = cursorLockPoint_;
		}
		else
		{
			cursorLockAnchored_ = false;
		}

		/// [EN] Mouse buttons are also read from the same Win32 virtual-key table fetched above, rather than a separate API call.
		/// [JP] マウスボタンも、別途 API 呼び出しをするのではなく、上で取得した同じ Win32 仮想キーテーブルから読み取る。
		static constexpr Int mouseVKeys[MOUSE_BUTTON_COUNT] =
		{
			VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2
		};
		for (Int index = 0; index < MOUSE_BUTTON_COUNT; ++index)
		{
			previousMouseState_[index] = currentMouseState_[index];
			currentMouseState_[index] = (keyboardState[mouseVKeys[index]] & 0x80) != 0;
		}

		/// [EN] Gamepad polling only runs while a gamepad is actually connected (gamepad_ is set by the hot-plug handling above).
		/// [JP] ゲームパッドのポーリングは、実際にゲームパッドが接続されている間のみ実行される（gamepad_ は上のホットプラグ処理によって設定される）。
		if (gamepad_)
		{
			/// [EN] SDL reads the pad even while another application is active, so the pad is treated as untouched then.
			/// [JP] SDL は別のアプリがアクティブな間もパッドを読めてしまうので、その間は触られていないものとして扱う。
			for (Int index = 0; index < GAMEPAD_BUTTON_COUNT; ++index)
			{
				previousGamepadState_[index] = currentGamepadState_[index];
				currentGamepadState_[index] = active && SDL_GetGamepadButton(gamepad_, static_cast<SDL_GamepadButton>(index));
			}
			/// [EN] SDL axes span the Sint16 range; dividing by 32768 maps them to [-1, 1], and the clamp keeps the one extra negative step (-32768) from going past -1.
			/// [JP] SDL の軸は Sint16 の範囲をとる。32768 で割ると [-1, 1] になり、負側に1つ多い値(-32768)が -1 を越えないよう Clamp で抑える。
			/// [EN] SDL reports stick-down as +Y, so Y is flipped here to store up as +Y, matching the directional-key composites.
			/// [JP] SDL はスティックの下方向を +Y で返すので、ここで Y を反転し、方向キー組と同じく上を +Y として保存する。
			Vector2& leftStick = gamepadStickAxis_[static_cast<Int>(GamepadStick::Left)];
			Vector2& rightStick = gamepadStickAxis_[static_cast<Int>(GamepadStick::Right)];
			leftStick = active ? Vector2(Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX)) / 32768.0f, -1.0f, 1.0f), -Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY)) / 32768.0f, -1.0f, 1.0f)) : Vector2(0.0f, 0.0f);
			rightStick = active ? Vector2(Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX)) / 32768.0f, -1.0f, 1.0f), -Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY)) / 32768.0f, -1.0f, 1.0f)) : Vector2(0.0f, 0.0f);

			/// [EN] SDL reports the triggers as axes rather than buttons, running from 0 (released) to 32767 (fully pulled), so the same mapping lands them in [0, 1].
			/// [JP] SDL はトリガーをボタンではなく軸として返し、0（離している）から 32767（引き切り）をとるので、同じ変換で [0, 1] に収まる。
			gamepadTriggerAxis_[static_cast<Int>(GamepadTrigger::Left)] = active ? Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)) / 32768.0f, 0.0f, 1.0f) : 0.0f;
			gamepadTriggerAxis_[static_cast<Int>(GamepadTrigger::Right)] = active ? Clamp(static_cast<Float>(SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)) / 32768.0f, 0.0f, 1.0f) : 0.0f;
		}
	}

	/**
	* [EN]
	* Closes the active gamepad (if any) and shuts down the SDL gamepad
	* subsystem.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクティブなゲームパッド（あれば）を閉じ、SDLのゲームパッド
	* サブシステムを終了する。
	*/
	void InputSystem::Finalize()
	{
		if (gamepad_)
		{
			SDL_CloseGamepad(gamepad_);
			gamepad_ = nullptr;
		}
		SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	}

	/**
	* [EN]
	* Returns whether key satisfies mode (current state, or a
	* rising/falling edge versus last frame). Unfiltered. Returns false if
	* key is out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* key が mode を満たすか（現在の状態、または前フレームとの
	* 立ち上がり/立ち下がりエッジ）を返す。絞り込みはしない。key が
	* 範囲外なら false。
	*/
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

	/**
	* [EN]
	* Returns whether mouse button button satisfies mode. Unfiltered.
	* Returns false if button is out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マウスボタン button が mode を満たすかを返す。絞り込みはしない。
	* button が範囲外なら false。
	*/
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

	/**
	* [EN]
	* Returns the cursor's movement since last frame, in screen pixels.
	* Unfiltered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前フレームからのカーソルの移動量を、画面ピクセル単位で返す。
	* 絞り込みはしない。
	*/
	Vector2 InputSystem::MouseMotion()
	{
		return mouseMotion_;
	}

	/**
	* [EN]
	* Returns the mouse wheel rotation accumulated last frame, in notches
	* (positive away from the user). Unfiltered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前フレームに累積されたマウスホイールの回転量を、ノッチ単位で返す
	* （奥へ回すと正）。絞り込みはしない。
	*/
	Float InputSystem::MouseWheel()
	{
		return currentMouseWheel_;
	}

	/**
	* [EN]
	* Adds delta notches of wheel rotation to the amount collected for the
	* frame in progress; the next Update() makes the total readable through
	* MouseWheel(). Called from the host's window procedure for every
	* WM_MOUSEWHEEL message, since the wheel has no state to poll.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 進行中のフレームで集めているホイールの回転量に、delta ノッチを足す。
	* 次の Update() で合計が MouseWheel() から読めるようになる。ホイールには
	* ポーリングできる状態が無いので、ホストのウィンドウプロシージャが
	* WM_MOUSEWHEEL メッセージのたびに呼ぶ。
	*/
	void InputSystem::MouseWheel(Float delta)
	{
		/// [EN] Added rather than assigned, so several messages arriving within one frame all count.
		/// [JP] 代入ではなく足し込むので、1フレームの間に届いた複数のメッセージが全て数えられる。
		pendingMouseWheel_ += delta;
	}

	/**
	* [EN]
	* Begins re-anchoring the cursor each frame (see Update()) so
	* camera-look style dragging never hits a monitor edge. No-op if
	* already captured.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 毎フレームカーソルを再アンカーする（Update() を参照）ことを
	* 開始し、カメラ視点ドラッグ操作がモニタ端に到達しないようにする。
	* 既にキャプチャ中なら何もしない。
	*/
	void InputSystem::BeginMouseCapture()
	{
		if (mouseCaptured_)
		{
			return;
		}

		mouseCaptured_ = true;

		/// [EN] Remember the cursor's current position both as the anchor Update() re-snaps to every frame and as the position EndMouseCapture() restores later.
		/// [JP] 現在のカーソル位置を、Update() が毎フレーム再スナップするアンカーとして、また後で EndMouseCapture() が復元する位置として記憶する。
		POINT origin;
		GetCursorPos(&origin);
		mouseCaptureReturn_ = Vector2(static_cast<Float>(origin.x), static_cast<Float>(origin.y));
		mouseCaptureAnchor_ = mouseCaptureReturn_;

		ShowCursor(FALSE);
	}

	/**
	* [EN]
	* Ends mouse capture, restoring the cursor to its position when
	* BeginMouseCapture() was called. No-op if not captured.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マウスキャプチャを終了し、カーソルを BeginMouseCapture() が
	* 呼ばれた時点の位置へ戻す。キャプチャ中でなければ何もしない。
	*/
	void InputSystem::EndMouseCapture()
	{
		if (!mouseCaptured_)
		{
			return;
		}

		mouseCaptured_ = false;

		ShowCursor(TRUE);
		SetCursorPos(static_cast<Int>(mouseCaptureReturn_.x), static_cast<Int>(mouseCaptureReturn_.y));
		mousePoint_ = mouseCaptureReturn_;
	}

	/**
	* [EN]
	* Locks the cursor in place for the game; Update() holds it from the
	* next frame on. No-op if already locked.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームのためにカーソルをその場に固定する。次のフレームから Update()
	* が留める。既にロック中なら何もしない。
	*/
	void InputSystem::LockCursor()
	{
		/// [EN] Only the request is recorded here; where to hold the cursor is decided by Update() on the first frame the lock applies.
		/// [JP] ここでは要求を記録するだけ。どこに留めるかは、ロックが効く最初のフレームで Update() が決める。
		if (cursorLocked_)
		{
			return;
		}

		cursorLocked_ = true;
		cursorLockPointFixed_ = false;
	}

	/**
	* [EN]
	* Locks the cursor at point (screen pixels); Update() holds it there
	* from the next frame on. Calling it again moves the lock to the new
	* point.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルを point（画面ピクセル）に固定する。次のフレームから
	* Update() がそこに留める。もう一度呼ぶと、固定する位置が新しい
	* point に移る。
	*/
	void InputSystem::LockCursor(Vector2 point)
	{
		/// [EN] The point is kept fixed, so Update() never replaces it with the cursor position, even after the lock pauses (e.g. the cursor leaving the game view in the editor).
		/// [JP] 位置は固定のままにするので、ロックが一時的に外れた後(例: エディタでカーソルがゲームビューを出た)も、Update() がカーソル位置で置き換えることは無い。
		cursorLocked_ = true;
		cursorLockPoint_ = point;
		cursorLockPointFixed_ = true;
	}

	/**
	* [EN]
	* Releases the cursor lock; the cursor stays where it is.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カーソルの固定を解く。カーソルはその場に残る。
	*/
	void InputSystem::UnlockCursor()
	{
		cursorLocked_ = false;
		cursorLockPointFixed_ = false;
		cursorLockAnchored_ = false;
	}

	/**
	* [EN]
	* Hides the cursor while it is over this application's windows. No-op
	* if already hidden.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このアプリのウィンドウ上にある間、カーソルを見えなくする。既に
	* 隠していれば何もしない。
	*/
	void InputSystem::HideCursor()
	{
		/// [EN] Win32 keeps a display counter that ShowCursor() raises and lowers, shared with mouse capture; guarding with cursorHidden_ keeps this pair to exactly one step, so hiding and capture never undo each other.
		/// [JP] Win32 は ShowCursor() で上下する表示カウンタを持ち、マウスキャプチャと共有している。cursorHidden_ で守ってこの対をちょうど1段にし、非表示とキャプチャが互いを打ち消さないようにする。
		if (cursorHidden_)
		{
			return;
		}

		cursorHidden_ = true;
		ShowCursor(FALSE);
	}

	/**
	* [EN]
	* Shows the cursor again after HideCursor(). No-op if not hidden.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* HideCursor() で隠したカーソルを再び表示する。隠していなければ何も
	* しない。
	*/
	void InputSystem::RevealCursor()
	{
		if (!cursorHidden_)
		{
			return;
		}

		cursorHidden_ = false;
		ShowCursor(TRUE);
	}

	/**
	* [EN]
	* Rumbles the gamepad body's low/high frequency motors for durationMs
	* milliseconds; all zero stops it. No-op if no gamepad is connected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームパッド本体の低周波/高周波モーターを durationMs ミリ秒間振動
	* させる。全て 0 なら止める。ゲームパッドが接続されていなければ何も
	* しない。
	*/
	void InputSystem::RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs)
	{
		if (gamepad_)
		{
			SDL_RumbleGamepad(gamepad_, lowFrequency, highFrequency, durationMs);
		}
	}

	/**
	* [EN]
	* Rumbles the gamepad's trigger motors (where supported) for
	* durationMs milliseconds; all zero stops it. No-op if no gamepad is
	* connected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームパッドのトリガーモーター（対応していれば）を durationMs
	* ミリ秒間振動させる。全て 0 なら止める。ゲームパッドが接続されて
	* いなければ何もしない。
	*/
	void InputSystem::RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs)
	{
		if (gamepad_)
		{
			SDL_RumbleGamepadTriggers(gamepad_, left, right, durationMs);
		}
	}

	/**
	* [EN]
	* Registers action with no bindings yet, if it doesn't already exist.
	* No-op if it does.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action をバインドなしで登録する（まだ存在しなければ）。既に
	* 存在していれば何もしない。
	*/
	void InputSystem::RegisterAction(String action)
	{
		if (actionBindings_.find(action) != actionBindings_.end())
		{
			return;
		}

		/// [EN] actionNameList_ records the action once, when it first appears, so ActionNameList() keeps first-registered order.
		/// [JP] actionNameList_ にはアクションが初めて現れたときに一度だけ記録し、ActionNameList() が最初に登録された順を保つようにする。
		actionNameList_.push_back(action);
		actionBindings_[action] = ActionBinding();
	}

	/**
	* [EN]
	* Removes action entirely (all its key/gamepad bindings).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action を（キー/ゲームパッドの全バインドごと）完全に削除する。
	*/
	void InputSystem::UnregisterAction(String action)
	{
		if (actionBindings_.erase(action))
		{
			SeedCore::erase(actionNameList_, action);
		}
	}

	/**
	* [EN]
	* Adds key to action's key bindings. No-op if already bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action のキーバインドに key を追加する。既に紐づいていれば何もしない。
	*/
	void InputSystem::BindKey(String action, Key key)
	{
		/// [EN] Binding to an unknown action registers it first, appending it to actionNameList_ so ActionNameList() keeps first-registered order.
		/// [JP] 未知のアクションへの割り当ては先にそのアクションを登録し、actionNameList_ に追加して ActionNameList() が最初に登録された順を保つようにする。
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

	/**
	* [EN]
	* Removes key from action's key bindings, if present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action のキーバインドから key を取り除く（存在すれば）。
	*/
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

	/**
	* [EN]
	* Returns the keys currently bound to action. Empty if action is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action に現在紐づいているキーの一覧を返す。action が未知なら空。
	*/
	const DynamicArray<InputSystem::Key>& InputSystem::BoundKey(String action)
	{
		static const DynamicArray<Key> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.keyList_ : empty;
	}

	/**
	* [EN]
	* Adds button to action's gamepad-button bindings. No-op if already bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action のゲームパッドボタンバインドに button を追加する。既に
	* 紐づいていれば何もしない。
	*/
	void InputSystem::BindGamepad(String action, SDL_GamepadButton button)
	{
		/// [EN] Binding to an unknown action registers it first, appending it to actionNameList_ so ActionNameList() keeps first-registered order.
		/// [JP] 未知のアクションへの割り当ては先にそのアクションを登録し、actionNameList_ に追加して ActionNameList() が最初に登録された順を保つようにする。
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

	/**
	* [EN]
	* Removes button from action's gamepad-button bindings, if present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action のゲームパッドボタンバインドから button を取り除く（存在すれば）。
	*/
	void InputSystem::UnbindGamepad(String action, SDL_GamepadButton button)
	{
		auto iterator = actionBindings_.find(action);
		if (iterator == actionBindings_.end())
		{
			return;
		}

		DynamicArray<SDL_GamepadButton>& gamepadList = iterator->second.gamepadList_;
		SeedCore::erase(gamepadList, button);
	}

	/**
	* [EN]
	* Returns the gamepad buttons currently bound to action. Empty if action is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action に現在紐づいているゲームパッドボタンの一覧を返す。action が
	* 未知なら空。
	*/
	const DynamicArray<SDL_GamepadButton>& InputSystem::BoundGamepad(String action)
	{
		static const DynamicArray<SDL_GamepadButton> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.gamepadList_ : empty;
	}

	/**
	* [EN]
	* Adds one WASD/arrow-key-style directional composite (up/down/
	* left/right) to action's axis bindings. No-op if the exact same four
	* keys are already bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action の軸バインドに、WASD/矢印キー的な方向キー1組（上下左右）を
	* 追加する。全く同じ4キーが既に紐づいていれば何もしない。
	*/
	void InputSystem::BindAxisKey(String action, DirectionalKey axisKey)
	{
		/// [EN] Binding to an unknown action registers it first, appending it to actionNameList_ so ActionNameList() keeps first-registered order.
		/// [JP] 未知のアクションへの割り当ては先にそのアクションを登録し、actionNameList_ に追加して ActionNameList() が最初に登録された順を保つようにする。
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

	/**
	* [EN]
	* Removes axisKey from action's axis bindings, if an exact match is present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action の軸バインドから axisKey を取り除く（完全一致するものがあれば）。
	*/
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

	/**
	* [EN]
	* Returns the directional-key composites currently bound to action. Empty if action is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action に現在紐づいている方向キー組の一覧を返す。action が未知なら空。
	*/
	const DynamicArray<InputSystem::DirectionalKey>& InputSystem::BoundAxisKey(String action)
	{
		static const DynamicArray<DirectionalKey> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.axisKeyList_ : empty;
	}

	/**
	* [EN]
	* Adds stick to action's bound analog sticks. No-op if already bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action の紐づくアナログスティックに stick を追加する。既に
	* 紐づいていれば何もしない。
	*/
	void InputSystem::BindStick(String action, GamepadStick stick)
	{
		/// [EN] Binding to an unknown action registers it first, appending it to actionNameList_ so ActionNameList() keeps first-registered order.
		/// [JP] 未知のアクションへの割り当ては先にそのアクションを登録し、actionNameList_ に追加して ActionNameList() が最初に登録された順を保つようにする。
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

	/**
	* [EN]
	* Removes stick from action's bound analog sticks, if present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action の紐づくアナログスティックから stick を取り除く（存在すれば）。
	*/
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

	/**
	* [EN]
	* Returns the analog sticks currently bound to action. Empty if action is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* action に現在紐づいているアナログスティックの一覧を返す。action が
	* 未知なら空。
	*/
	const DynamicArray<InputSystem::GamepadStick>& InputSystem::BoundStick(String action)
	{
		static const DynamicArray<GamepadStick> empty;
		auto iterator = actionBindings_.find(action);
		return iterator != actionBindings_.end() ? iterator->second.stickList_ : empty;
	}

	/**
	* [EN]
	* Returns every known action name, in the order each was first
	* registered (by RegisterAction() or the first binding to it).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 既知の全アクション名を、最初に登録された順（RegisterAction() か、
	* 最初の割り当てのどちらか）で返す。
	*/
	const DynamicArray<String>& InputSystem::ActionNameList()
	{
		return actionNameList_;
	}

	/**
	* [EN]
	* Loads action bindings from path, replacing the current table.
	* Missing file or unparseable fields are silently ignored.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* path からアクションバインドを読み込み、現在のテーブルを置き換える。
	* ファイルが無い、またはフィールドが解釈できない場合は何もせず
	* 無視する。
	*/
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
			/// [EN] Records are saved in first-registered order, so appending each new action to actionNameList_ restores that order.
			/// [JP] レコードは最初に登録された順で保存されているので、新しいアクションを順に actionNameList_ へ追加すればその順に戻る。
			if (actionBindings_.find(record.action_) == actionBindings_.end())
			{
				actionNameList_.push_back(record.action_);
			}
			ActionBinding& binding = actionBindings_[record.action_];

			std::ranges::transform(record.keyList_, std::back_inserter(binding.keyList_), [](Int32 key) { return static_cast<Key>(key); });
			std::ranges::transform(record.gamepadList_, std::back_inserter(binding.gamepadList_), [](Int32 button) { return static_cast<SDL_GamepadButton>(button); });

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

	/**
	* [EN]
	* Saves the current action binding table to path.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のアクションバインドテーブルを path へ保存する。
	*/
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
			std::ranges::transform(binding.gamepadList_, std::back_inserter(record.gamepadList_), [](SDL_GamepadButton button) { return static_cast<Int32>(button); });

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
