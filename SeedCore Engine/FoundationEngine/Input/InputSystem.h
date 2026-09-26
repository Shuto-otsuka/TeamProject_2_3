#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	/**
	* [EN]
	* Engine- and editor-side input backend. Polls raw keyboard (Win32),
	* mouse (Win32) and gamepad (SDL) state once per frame via Update(),
	* owns the action binding table, and offers the host-only operations
	* (Initialize/Update/Finalize, and MouseWheel(delta) to hand over
	* wheel rotation), mouse capture, and binding editing. Its state
	* queries are unfiltered, for editor tools.
	* Gameplay code reads input through Input instead, which filters out
	* the frames Update() was told not to hand to the game.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンジン・エディタ側の入力バックエンド。生のキーボード（Win32）、
	* マウス（Win32）、ゲームパッド（SDL）の状態を Update() で毎フレーム
	* 1回ポーリングし、アクションバインドの表を持つ。ホスト専用の操作
	* （Initialize/Update/Finalize と、ホイールの回転量を渡す
	* MouseWheel(delta)）、マウスキャプチャ、バインドの編集もここにある。
	* 状態の問い合わせは絞り込みをしない、エディタのツール向けのもの。
	* ゲームプレイのコードは代わりに Input を
	* 通して読み、Update() がゲームに渡さないと指定したフレームは Input が
	* 取り除く。
	*/
	class SEEDCORE_API InputSystem
	{
	public:
		/**
		* [EN]
		* Edge-triggering mode for state queries (KeyState/MouseState/GamepadState).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 状態クエリ（KeyState/MouseState/GamepadState）のエッジトリガーモード。
		*/
		enum class TriggerMode
		{
			/// [EN] Query the current held state, regardless of last frame.
			/// [JP] 前フレームに関わらず、現在の押下状態を問い合わせる。
			NONE,

			/// [EN] True only on the frame the input transitions from up to down.
			/// [JP] 入力が離れた状態から押された状態へ遷移したフレームのみ true。
			RISING_EDGE,

			/// [EN] True only on the frame the input transitions from down to up.
			/// [JP] 入力が押された状態から離れた状態へ遷移したフレームのみ true。
			FALLING_EDGE
		};

		/**
		* [EN]
		* Readable keyboard key codes for use at KeyState() call sites, in
		* place of raw Win32 virtual-key codes. Each value is the Win32
		* virtual-key code itself, so it indexes the key table directly.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KeyState() の呼び出し箇所で、生のWin32仮想キーコードの代わりに
		* 使う、読みやすいキーボードキーコード。各値は Win32 の仮想キー
		* コードそのものなので、キー表をそのまま引ける。
		*/
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

		/**
		* [EN]
		* Readable mouse button codes for use at MouseState() call sites, in
		* place of raw button indices. Extra1/Extra2 are the side (back/
		* forward) buttons.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* MouseState() の呼び出し箇所で、生のボタン番号の代わりに使う、
		* 読みやすいマウスボタンコード。Extra1/Extra2 はサイドボタン
		* （戻る/進む）。
		*/
		enum class MouseButton : Int
		{
			Left,
			Right,
			Middle,
			Extra1,
			Extra2
		};

		/**
		* [EN]
		* Readable aliases for GamepadState() call sites, in place of raw
		* SDL_GamepadButton values. A/B/X/Y follow the Xbox face-button
		* layout (A is the bottom button).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GamepadState() の呼び出し箇所で、生の SDL_GamepadButton 値の
		* 代わりに使う、読みやすいエイリアス。A/B/X/Y は Xbox の配置に
		* 従う（A が下のボタン）。
		*/
		struct GamepadButton
		{
			static constexpr SDL_GamepadButton A = SDL_GAMEPAD_BUTTON_SOUTH;
			static constexpr SDL_GamepadButton B = SDL_GAMEPAD_BUTTON_EAST;
			static constexpr SDL_GamepadButton X = SDL_GAMEPAD_BUTTON_WEST;
			static constexpr SDL_GamepadButton Y = SDL_GAMEPAD_BUTTON_NORTH;
			static constexpr SDL_GamepadButton LeftShoulder = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
			static constexpr SDL_GamepadButton RightShoulder = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
			static constexpr SDL_GamepadButton LeftStick = SDL_GAMEPAD_BUTTON_LEFT_STICK;
			static constexpr SDL_GamepadButton RightStick = SDL_GAMEPAD_BUTTON_RIGHT_STICK;
			static constexpr SDL_GamepadButton DPadUp = SDL_GAMEPAD_BUTTON_DPAD_UP;
			static constexpr SDL_GamepadButton DPadDown = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
			static constexpr SDL_GamepadButton DPadLeft = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
			static constexpr SDL_GamepadButton DPadRight = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
			static constexpr SDL_GamepadButton Start = SDL_GAMEPAD_BUTTON_START;
			static constexpr SDL_GamepadButton Back = SDL_GAMEPAD_BUTTON_BACK;

		private:
			GamepadButton() = delete;
		};

		/**
		* [EN]
		* Which analog stick to read (see Input::GamepadAxis()) or bind to
		* an action (see BindStick()/Input::ActionAxis()). The value also
		* indexes the per-stick tilt table.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* どちらのアナログスティックを読むか（Input::GamepadAxis() 参照）、
		* またはアクションに割り当てるか（BindStick()/Input::ActionAxis()
		* 参照）。値はスティックごとの傾きの表の添字にもなる。
		*/
		enum class GamepadStick
		{
			Left,
			Right
		};

		/**
		* [EN]
		* Which analog trigger (LT/RT, L2/R2) to read (see
		* Input::GamepadAxis()). The value also indexes the per-trigger
		* pull table.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* どちらのアナログトリガー（LT/RT、L2/R2）を読むか
		* （Input::GamepadAxis() 参照）。値はトリガーごとの引き具合の表の
		* 添字にもなる。
		*/
		enum class GamepadTrigger
		{
			Left,
			Right
		};

		/**
		* [EN]
		* One WASD/arrow-key style directional composite: four digital keys combined into a Vector2 by Input::ActionAxis().
		* An action can have several of these bound (e.g. both WASD and the arrow keys).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* WASD/矢印キー型の方向キー1組。4つのデジタルキーを Input::ActionAxis() が Vector2 に合成する。
		* 1つのアクションに複数組を割り当てられる(例: WASD と矢印キーの両方)。
		*/
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
		/// [EN] Readable alias for TriggerMode::NONE, for use at KeyState/MouseState/GamepadState call sites.
		/// [JP] TriggerMode::NONE の読みやすいエイリアス。KeyState/MouseState/GamepadState の呼び出し箇所で使う。
		static constexpr TriggerMode IsPressed = TriggerMode::NONE;

		/// [EN] Readable alias for TriggerMode::RISING_EDGE.
		/// [JP] TriggerMode::RISING_EDGE の読みやすいエイリアス。
		static constexpr TriggerMode OnPressed = TriggerMode::RISING_EDGE;

		/// [EN] Readable alias for TriggerMode::FALLING_EDGE.
		/// [JP] TriggerMode::FALLING_EDGE の読みやすいエイリアス。
		static constexpr TriggerMode OnReleased = TriggerMode::FALLING_EDGE;

	public:
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
		static void Initialize();

		/**
		* [EN]
		* Polls keyboard/mouse/gamepad state for the current frame, rotating
		* the previous-frame snapshot forward. Call once per frame.
		* gameInput says whether this frame's keyboard/mouse input belongs
		* to the game (Input reports nothing for them otherwise); gamepad
		* input always does. While the application is not the active one,
		* everything is polled as released and still.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在フレームのキーボード/マウス/ゲームパッド状態をポーリングし、
		* 前フレームのスナップショットを繰り越す。毎フレーム1回呼び出す。
		* gameInput は、このフレームのキーボード/マウス入力をゲームに渡すか
		* を表す（渡さない場合、Input はそれらを何も無いものとして返す）。
		* ゲームパッドの入力は常に渡す。アプリがアクティブでない間は、
		* 全て押されていない・動いていないものとして取り込む。
		*/
		static void Update(Bool gameInput);

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
		static void Finalize();

	public:
		/**
		* [EN]
		* Returns whether key satisfies mode (current state, or a
		* rising/falling edge versus last frame). Unfiltered. Returns false
		* if key is out of range.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* key が mode を満たすか（現在の状態、または前フレームとの
		* 立ち上がり/立ち下がりエッジ）を返す。絞り込みはしない。key が
		* 範囲外なら false。
		*/
		static Bool KeyState(Key key, TriggerMode mode = TriggerMode::NONE);

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
		static Bool MouseState(MouseButton button, TriggerMode mode = TriggerMode::NONE);

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
		static Vector2 MouseMotion();

		/**
		* [EN]
		* Returns the mouse wheel rotation accumulated last frame, in
		* notches (positive away from the user). Unfiltered.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前フレームに累積されたマウスホイールの回転量を、ノッチ単位で返す
		* （奥へ回すと正）。絞り込みはしない。
		*/
		static Float MouseWheel();

		/**
		* [EN]
		* Adds delta notches of wheel rotation to the amount collected for
		* the frame in progress; the next Update() makes the total readable
		* through MouseWheel(). The wheel has no state Win32 can be polled
		* for - each turn only arrives as a WM_MOUSEWHEEL message to the
		* focused window - so the host's window procedure forwards every
		* such message here. Several messages in one frame add up.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 進行中のフレームで集めているホイールの回転量に、delta ノッチを
		* 足す。次の Update() で合計が MouseWheel() から読めるようになる。
		* ホイールには Win32 でポーリングできる状態が無く、回すたびに
		* フォーカスのあるウィンドウへ WM_MOUSEWHEEL メッセージとして届く
		* だけなので、ホストのウィンドウプロシージャがそのメッセージを
		* 全てここへ渡す。1フレームに複数届いた分は足し合わされる。
		*/
		static void MouseWheel(Float delta);

	public:
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
		static void BeginMouseCapture();

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
		static void EndMouseCapture();

	public:
		/**
		* [EN]
		* Locks the cursor in place for the game: from the next Update()
		* on, the cursor is put back every frame to where it was when the
		* lock took effect, so it never drifts or leaves the window while
		* MouseMotion() keeps reporting how far the mouse was moved (e.g.
		* for camera look). Only holds on frames whose input goes to the
		* game and while mouse capture is not active; combine with
		* HideCursor() to also hide it. No-op if already locked.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームのためにカーソルをその場に固定する。次の Update() から、
		* ロックが効き始めたときの位置へ毎フレームカーソルを戻すので、
		* カーソルはずれたりウィンドウから出たりせず、MouseMotion() は
		* マウスを動かした量を返し続ける（例: 視点操作）。固定するのは
		* 入力をゲームに渡すフレームで、マウスキャプチャ中でない間だけ。
		* 見えなくもしたいときは HideCursor() と組み合わせる。既に
		* ロック中なら何もしない。
		*/
		static void LockCursor();

		/**
		* [EN]
		* Same as LockCursor(), but holds the cursor at point (screen
		* pixels, the same space as Input::MousePoint()) instead of where
		* it happens to be. Calling it again moves the lock to the new
		* point.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LockCursor() と同じだが、たまたまある位置ではなく point（画面
		* ピクセル。Input::MousePoint() と同じ座標）にカーソルを留める。
		* もう一度呼ぶと、固定する位置が新しい point に移る。
		*/
		static void LockCursor(Vector2 point);

		/**
		* [EN]
		* Releases the lock set by LockCursor(); the cursor stays where it
		* is. No-op if not locked.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LockCursor() による固定を解く。カーソルはその場に残る。ロック中で
		* なければ何もしない。
		*/
		static void UnlockCursor();

		/**
		* [EN]
		* Hides the cursor while it is over this application's windows.
		* Position, movement and buttons are still reported as usual.
		* No-op if already hidden.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このアプリのウィンドウ上にある間、カーソルを見えなくする。位置・
		* 移動量・ボタンはこれまで通り返す。既に隠していれば何もしない。
		*/
		static void HideCursor();

		/**
		* [EN]
		* Shows the cursor again after HideCursor(). No-op if not hidden.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* HideCursor() で隠したカーソルを再び表示する。隠していなければ
		* 何もしない。
		*/
		static void RevealCursor();

	public:
		/**
		* [EN]
		* Rumbles the gamepad body's low/high frequency motors (0-65535
		* each) for durationMs milliseconds; all zero stops it. No-op if no
		* gamepad is connected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームパッド本体の低周波/高周波モーター（それぞれ 0〜65535）を
		* durationMs ミリ秒間振動させる。全て 0 なら止める。ゲームパッドが
		* 接続されていなければ何もしない。
		*/
		static void RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs);

		/**
		* [EN]
		* Rumbles the gamepad's trigger motors (0-65535 each, where
		* supported) for durationMs milliseconds; all zero stops it. No-op
		* if no gamepad is connected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームパッドのトリガーモーター（それぞれ 0〜65535、対応して
		* いれば）を durationMs ミリ秒間振動させる。全て 0 なら止める。
		* ゲームパッドが接続されていなければ何もしない。
		*/
		static void RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs);

	public:
		/**
		* [EN]
		* Registers action with no bindings yet, if it doesn't already
		* exist. No-op if it does.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action をバインドなしで登録する（まだ存在しなければ）。既に
		* 存在していれば何もしない。
		*/
		static void RegisterAction(String action);

		/**
		* [EN]
		* Removes action entirely (all its key/gamepad bindings).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action を（キー/ゲームパッドの全バインドごと）完全に削除する。
		*/
		static void UnregisterAction(String action);

		/**
		* [EN]
		* Adds key to action's key bindings. No-op if already bound.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action のキーバインドに key を追加する。既に紐づいていれば何もしない。
		*/
		static void BindKey(String action, Key key);

		/**
		* [EN]
		* Removes key from action's key bindings, if present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action のキーバインドから key を取り除く（存在すれば）。
		*/
		static void UnbindKey(String action, Key key);

		/**
		* [EN]
		* Returns the keys currently bound to action. Empty if action is unknown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action に現在紐づいているキーの一覧を返す。action が未知なら空。
		*/
		static const DynamicArray<Key>& BoundKey(String action);

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
		static void BindGamepad(String action, SDL_GamepadButton button);

		/**
		* [EN]
		* Removes button from action's gamepad-button bindings, if present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action のゲームパッドボタンバインドから button を取り除く（存在すれば）。
		*/
		static void UnbindGamepad(String action, SDL_GamepadButton button);

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
		static const DynamicArray<SDL_GamepadButton>& BoundGamepad(String action);

		/**
		* [EN]
		* Adds one WASD/arrow-key-style directional composite (up/down/
		* left/right) to action's axis bindings. No-op if the exact same
		* four keys are already bound.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action の軸バインドに、WASD/矢印キー的な方向キー1組（上下左右）を
		* 追加する。全く同じ4キーが既に紐づいていれば何もしない。
		*/
		static void BindAxisKey(String action, DirectionalKey axisKey);

		/**
		* [EN]
		* Removes axisKey from action's axis bindings, if an exact match is present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action の軸バインドから axisKey を取り除く（完全一致するものがあれば）。
		*/
		static void UnbindAxisKey(String action, DirectionalKey axisKey);

		/**
		* [EN]
		* Returns the directional-key composites currently bound to action. Empty if action is unknown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action に現在紐づいている方向キー組の一覧を返す。action が未知なら空。
		*/
		static const DynamicArray<DirectionalKey>& BoundAxisKey(String action);

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
		static void BindStick(String action, GamepadStick stick);

		/**
		* [EN]
		* Removes stick from action's bound analog sticks, if present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action の紐づくアナログスティックから stick を取り除く（存在すれば）。
		*/
		static void UnbindStick(String action, GamepadStick stick);

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
		static const DynamicArray<GamepadStick>& BoundStick(String action);

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
		static const DynamicArray<String>& ActionNameList();

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
		static void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/InputBindings.scg");

		/**
		* [EN]
		* Saves the current action binding table to path.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のアクションバインドテーブルを path へ保存する。
		*/
		static void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/InputBindings.scg");

	private:
		/// [EN] Input reads the polled state and the binding table directly, filtering by gameInput_ where needed.
		/// [JP] Input はポーリングした状態とバインド表を直接読み、必要な所で gameInput_ によって絞り込む。
		friend class Input;

		/**
		* [EN]
		* One action's bound keys, gamepad buttons, directional-key
		* composites and analog sticks.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1つのアクションに紐づくキー、ゲームパッドボタン、方向キー組、
		* アナログスティック。
		*/
		struct ActionBinding
		{
			/// [EN] Keys bound to this action (see BoundKey()).
			/// [JP] このアクションに紐づくキー（BoundKey() 参照）。
			DynamicArray<Key> keyList_;

			/// [EN] Gamepad buttons bound to this action (see BoundGamepad()).
			/// [JP] このアクションに紐づくゲームパッドボタン（BoundGamepad() 参照）。
			DynamicArray<SDL_GamepadButton> gamepadList_;

			/// [EN] Directional-key composites bound to this action (see BoundAxisKey()/Input::ActionAxis()).
			/// [JP] このアクションに紐づく方向キー組（BoundAxisKey()/Input::ActionAxis() 参照）。
			DynamicArray<DirectionalKey> axisKeyList_;

			/// [EN] Analog sticks bound to this action (see BoundStick()/Input::ActionAxis()).
			/// [JP] このアクションに紐づくアナログスティック（BoundStick()/Input::ActionAxis() 参照）。
			DynamicArray<GamepadStick> stickList_;
		};

		InputSystem() = delete;

	private:
		/// [EN] Number of tracked virtual-key slots.
		/// [JP] 追跡する仮想キースロットの数。
		static constexpr Int KEY_COUNT = 256;

		/// [EN] Number of tracked mouse buttons.
		/// [JP] 追跡するマウスボタンの数。
		static constexpr Int MOUSE_BUTTON_COUNT = 5;

		/// [EN] Number of tracked gamepad buttons.
		/// [JP] 追跡するゲームパッドボタンの数。
		static constexpr Int GAMEPAD_BUTTON_COUNT = SDL_GAMEPAD_BUTTON_COUNT;

		/// [EN] Whether this frame's keyboard/mouse input belongs to the game (see Update()); read by Input.
		/// [JP] このフレームのキーボード/マウス入力をゲームに渡すか（Update() 参照）。Input が読む。
		static Bool gameInput_;

		/// [EN] This frame's key-down state, indexed by virtual-key code (see KeyState()).
		/// [JP] 現在フレームのキー押下状態。仮想キーコードでインデックスする（KeyState() 参照）。
		static Bool currentKeyState_[KEY_COUNT];

		/// [EN] Last frame's key-down state, compared against currentKeyState_ to detect edges.
		/// [JP] 前フレームのキー押下状態。currentKeyState_ と比べてエッジを検出する。
		static Bool previousKeyState_[KEY_COUNT];

		/// [EN] This frame's mouse button state (see MouseState()).
		/// [JP] 現在フレームのマウスボタン状態（MouseState() 参照）。
		static Bool currentMouseState_[MOUSE_BUTTON_COUNT];

		/// [EN] Last frame's mouse button state, compared against currentMouseState_ to detect edges.
		/// [JP] 前フレームのマウスボタン状態。currentMouseState_ と比べてエッジを検出する。
		static Bool previousMouseState_[MOUSE_BUTTON_COUNT];

		/// [EN] Cursor's current position, in screen pixels (see Input::MousePoint()).
		/// [JP] カーソルの現在の位置。画面ピクセル単位（Input::MousePoint() 参照）。
		static Vector2 mousePoint_;

		/// [EN] Cursor's movement since last frame, in screen pixels (see MouseMotion()).
		/// [JP] 前フレームからのカーソルの移動量。画面ピクセル単位（MouseMotion() 参照）。
		static Vector2 mouseMotion_;

		/// [EN] Mouse wheel rotation during the last completed frame, in notches (see MouseWheel()).
		/// [JP] 直近に完了したフレーム中のマウスホイールの回転量。ノッチ単位（MouseWheel() 参照）。
		static Float currentMouseWheel_;

		/// [EN] Mouse wheel rotation pushed since the last Update() (see MouseWheel(Float)); becomes currentMouseWheel_ at the next Update().
		/// [JP] 直近の Update() 以降に送られたマウスホイールの回転量（MouseWheel(Float) 参照）。次の Update() で currentMouseWheel_ になる。
		static Float pendingMouseWheel_;

		/// [EN] Whether mouse capture is active; BeginMouseCapture()/EndMouseCapture() check it so calling either twice does nothing.
		/// [JP] マウスキャプチャ中かどうか。BeginMouseCapture()/EndMouseCapture() がこれを見るので、どちらを2回呼んでも何も起きない。
		static Bool mouseCaptured_;

		/// [EN] While captured the cursor is warped here every frame so it never hits a monitor edge (see Update()).
		/// [JP] キャプチャ中はカーソルを毎フレームここへ戻し、モニタ端に届かないようにする（Update() 参照）。
		static Vector2 mouseCaptureAnchor_;

		/// [EN] Where the cursor was when capture began; EndMouseCapture() puts it back here.
		/// [JP] キャプチャを始めたときのカーソル位置。EndMouseCapture() がここへ戻す。
		static Vector2 mouseCaptureReturn_;

		/// [EN] Whether the game asked for the cursor to be locked (see LockCursor()).
		/// [JP] ゲームがカーソルの固定を求めているか（LockCursor() 参照）。
		static Bool cursorLocked_;

		/// [EN] Where the locked cursor is put back every frame, in screen pixels: the point given to LockCursor(Vector2), or else the cursor position on the frame the lock took effect.
		/// [JP] ロック中のカーソルを毎フレーム戻す位置（画面ピクセル）。LockCursor(Vector2) に渡された位置か、そうでなければロックが効き始めたフレームのカーソル位置。
		static Vector2 cursorLockPoint_;

		/// [EN] Whether cursorLockPoint_ was given by LockCursor(Vector2) and so stays fixed, rather than being taken from the cursor.
		/// [JP] cursorLockPoint_ が LockCursor(Vector2) で指定されたもので、カーソルから取り直さずに固定のままか。
		static Bool cursorLockPointFixed_;

		/// [EN] Whether the lock is currently holding the cursor; cleared on any frame the lock does not apply, so without a fixed point the next frame it applies takes the cursor position afresh.
		/// [JP] ロックが今カーソルを留めているか。ロックが効かないフレームで下ろすので、位置が固定でなければ次に効くフレームでカーソル位置を取り直す。
		static Bool cursorLockAnchored_;

		/// [EN] Whether the cursor is hidden (see HideCursor()).
		/// [JP] カーソルを隠しているか（HideCursor() 参照）。
		static Bool cursorHidden_;

		/// [EN] This frame's gamepad button state (see Input::GamepadState()).
		/// [JP] 現在フレームのゲームパッドボタン状態（Input::GamepadState() 参照）。
		static Bool currentGamepadState_[GAMEPAD_BUTTON_COUNT];

		/// [EN] Last frame's gamepad button state, compared against currentGamepadState_ to detect edges.
		/// [JP] 前フレームのゲームパッドボタン状態。currentGamepadState_ と比べてエッジを検出する。
		static Bool previousGamepadState_[GAMEPAD_BUTTON_COUNT];

		/// [EN] Each stick's tilt, indexed by GamepadStick, each axis in [-1, 1] with up as +Y (see Input::GamepadAxis()).
		/// [JP] 各スティックの倒し具合。GamepadStick で引き、各軸 [-1, 1]、上が +Y（Input::GamepadAxis() 参照）。
		static Vector2 gamepadStickAxis_[2];

		/// [EN] Each trigger's pull, indexed by GamepadTrigger, in [0, 1] (see Input::GamepadAxis()).
		/// [JP] 各トリガーの引き具合。GamepadTrigger で引き、[0, 1]（Input::GamepadAxis() 参照）。
		static Float gamepadTriggerAxis_[2];

		/// [EN] Handle to the first connected gamepad, or nullptr if none.
		/// [JP] 最初に接続されたゲームパッドへのハンドル。なければ nullptr。
		static SDL_Gamepad* gamepad_;

		/// [EN] Action name -> its bound keys/gamepad buttons/directional keys/sticks.
		/// [JP] アクション名 -> 紐づくキー/ゲームパッドボタン/方向キー組/スティック。
		static FlatMap<String, ActionBinding> actionBindings_;

		/// [EN] Action names in first-registered order, for stable enumeration (see ActionNameList()).
		/// [JP] 最初に登録された順のアクション名。安定した列挙のため（ActionNameList() 参照）。
		static DynamicArray<String> actionNameList_;
	};
}
