#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Input/InputSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Gameplay-facing input API: what game code uses to read the
	* keyboard, mouse, gamepad and bound actions, and to rumble the
	* gamepad. Reads the state InputSystem polled this frame. Keyboard and
	* mouse input is reported as nothing pressed / no movement on frames
	* InputSystem::Update() was told not to hand to the game (e.g. in the
	* editor while the cursor is off the game view); gamepad input is
	* always reported. Host-only operations, mouse capture and binding
	* editing live on InputSystem.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームプレイ向けの入力API。ゲームのコードがキーボード・マウス・
	* ゲームパッド・割り当て済みのアクションを読み、ゲームパッドを振動
	* させるのに使う。InputSystem がこのフレームにポーリングした状態を
	* 読む。InputSystem::Update() がゲームに渡さないと指定したフレーム
	* （例: エディタでカーソルがゲームビューの外にある間）は、キーボードと
	* マウスの入力を「何も押されていない・動いていない」として返す。
	* ゲームパッドの入力は常に返す。ホスト専用の操作、マウスキャプチャ、
	* バインドの編集は InputSystem にある。
	*/
	class SEEDCORE_API Input
	{
	public:
		/// [EN] Edge-triggering mode for state queries; see InputSystem::TriggerMode.
		/// [JP] 状態クエリのエッジトリガーモード。InputSystem::TriggerMode を参照。
		using TriggerMode = InputSystem::TriggerMode;

		/// [EN] Readable keyboard key codes for KeyState(); see InputSystem::Key.
		/// [JP] KeyState() に渡す読みやすいキーボードキーコード。InputSystem::Key を参照。
		using Key = InputSystem::Key;

		/// [EN] Readable mouse button codes for MouseState(); see InputSystem::MouseButton.
		/// [JP] MouseState() に渡す読みやすいマウスボタンコード。InputSystem::MouseButton を参照。
		using MouseButton = InputSystem::MouseButton;

		/// [EN] Readable gamepad button aliases for GamepadState(); see InputSystem::GamepadButton.
		/// [JP] GamepadState() に渡す読みやすいゲームパッドボタンの別名。InputSystem::GamepadButton を参照。
		using GamepadButton = InputSystem::GamepadButton;

		/// [EN] Which analog stick GamepadAxis() reads; see InputSystem::GamepadStick.
		/// [JP] GamepadAxis() が読むアナログスティック。InputSystem::GamepadStick を参照。
		using GamepadStick = InputSystem::GamepadStick;

		/// [EN] Which analog trigger GamepadAxis() reads; see InputSystem::GamepadTrigger.
		/// [JP] GamepadAxis() が読むアナログトリガー。InputSystem::GamepadTrigger を参照。
		using GamepadTrigger = InputSystem::GamepadTrigger;

		/// [EN] Four-key directional composite used by action bindings; see InputSystem::DirectionalKey.
		/// [JP] アクションの割り当てで使う4キーの方向キー組。InputSystem::DirectionalKey を参照。
		using DirectionalKey = InputSystem::DirectionalKey;

		/// [EN] Readable alias for TriggerMode::NONE: held this frame.
		/// [JP] TriggerMode::NONE の読みやすいエイリアス。このフレームに押されている。
		static constexpr TriggerMode IsPressed = TriggerMode::NONE;

		/// [EN] Readable alias for TriggerMode::RISING_EDGE: pressed this frame.
		/// [JP] TriggerMode::RISING_EDGE の読みやすいエイリアス。このフレームに押された。
		static constexpr TriggerMode OnPressed = TriggerMode::RISING_EDGE;

		/// [EN] Readable alias for TriggerMode::FALLING_EDGE: released this frame.
		/// [JP] TriggerMode::FALLING_EDGE の読みやすいエイリアス。このフレームに離された。
		static constexpr TriggerMode OnReleased = TriggerMode::FALLING_EDGE;

	public:
		/**
		* [EN]
		* Returns whether key satisfies mode (held, pressed this frame, or
		* released this frame). False on frames not handed to the game.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* key が mode を満たすか（押している／このフレームに押した／
		* このフレームに離した）を返す。ゲームに渡さないフレームでは false。
		*/
		static Bool KeyState(Key key, TriggerMode mode = TriggerMode::NONE);

		/**
		* [EN]
		* Returns whether mouse button button satisfies mode. False on
		* frames not handed to the game.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マウスボタン button が mode を満たすかを返す。ゲームに渡さない
		* フレームでは false。
		*/
		static Bool MouseState(MouseButton button, TriggerMode mode = TriggerMode::NONE);

		/**
		* [EN]
		* Returns whether gamepad button button satisfies mode. Reported on
		* every frame, including ones whose keyboard/mouse input is not
		* handed to the game.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームパッドボタン button が mode を満たすかを返す。キーボード/
		* マウスの入力をゲームに渡さないフレームも含め、毎フレーム返す。
		*/
		static Bool GamepadState(SDL_GamepadButton button, TriggerMode mode = TriggerMode::NONE);

		/**
		* [EN]
		* Returns whether any key or gamepad button bound to action
		* satisfies mode, so game code doesn't branch on which device the
		* player is using. Keys follow KeyState(), buttons GamepadState().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action に紐づくいずれかのキーまたはゲームパッドボタンが mode を
		* 満たすかを返す。ゲームのコードはプレイヤーの使用デバイスで分岐
		* せずに済む。キーは KeyState()、ボタンは GamepadState() に従う。
		*/
		static Bool ActionState(String action, TriggerMode mode = TriggerMode::NONE);

		/**
		* [EN]
		* Returns the cursor's current position, in screen pixels.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カーソルの現在の位置を、画面ピクセル単位で返す。
		*/
		static Vector2 MousePoint();

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
		static Vector2 MouseMotion();

		/**
		* [EN]
		* Returns the mouse wheel rotation during last frame, in notches
		* (positive away from the user). Zero on frames not handed to the
		* game.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前フレーム中のマウスホイールの回転量を、ノッチ単位で返す（奥へ
		* 回すと正）。ゲームに渡さないフレームでは 0。
		*/
		static Float MouseWheel();

		/**
		* [EN]
		* Returns stick's tilt, each axis in [-1, 1], with up as +Y (the
		* same orientation as ActionAxis()).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* stick の倒し具合を、各軸 [-1, 1] で返す。上が +Y（ActionAxis() と
		* 同じ向き）。
		*/
		static Vector2 GamepadAxis(GamepadStick stick);

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
		static Float GamepadAxis(GamepadTrigger trigger);

		/**
		* [EN]
		* Returns action's combined 2D input: the bound directional-key
		* composites that are held, normalized; if none are held, the first
		* bound stick reporting nonzero tilt; otherwise (0, 0). Up is +Y.
		* Lets movement code read one action regardless of whether the
		* player is on keyboard or gamepad.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* action の入力を合成した2次元の値を返す: 紐づく方向キー組のうち
		* 押されているものを正規化して返す。どれも押されていなければ、
		* 紐づくスティックのうち最初に非ゼロの傾きを報告したものを返す。
		* どちらも無ければ (0, 0)。上が +Y。移動処理側はプレイヤーが
		* キーボードかゲームパッドかを問わず、1つのアクションを読むだけで済む。
		*/
		static Vector2 ActionAxis(String action);

		/**
		* [EN]
		* Rumbles the gamepad body's low/high frequency motors (0-65535
		* each) for durationMs milliseconds; all zero stops it. The low
		* motor gives a heavy shake, the high motor a fine buzz. No-op if no
		* gamepad is connected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームパッド本体の低周波/高周波モーター（それぞれ 0〜65535）を
		* durationMs ミリ秒間振動させる。全て 0 なら止める。低周波は重い
		* 揺れ、高周波は細かい振動になる。ゲームパッドが接続されていなければ
		* 何もしない。
		*/
		static void RumbleBody(Uint16 lowFrequency, Uint16 highFrequency, Uint32 durationMs);

		/**
		* [EN]
		* Rumbles the motors inside the left/right triggers (0-65535 each,
		* on controllers that have them) for durationMs milliseconds; all
		* zero stops it. No-op if no gamepad is connected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 左右トリガーの中のモーター（それぞれ 0〜65535、搭載している
		* コントローラーのみ）を durationMs ミリ秒間振動させる。全て 0 なら
		* 止める。ゲームパッドが接続されていなければ何もしない。
		*/
		static void RumbleTrigger(Uint16 left, Uint16 right, Uint32 durationMs);

	public:
		/**
		* [EN]
		* Locks the cursor where it is, so it never drifts or leaves the
		* window while MouseMotion() keeps reporting how far the mouse was
		* moved; for camera look. Combine with HideCursor() to also hide it.
		* In the editor the lock holds only while the cursor is over the
		* game view, and stopping play always releases it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カーソルをその場に固定する。カーソルはずれたりウィンドウから
		* 出たりせず、MouseMotion() はマウスを動かした量を返し続ける。
		* 視点操作向け。見えなくもしたいときは HideCursor() と組み合わせる。
		* エディタではカーソルがゲームビュー上にある間だけ固定し、プレイを
		* 止めると必ず解除される。
		*/
		static void LockCursor();

		/**
		* [EN]
		* Same as LockCursor(), but holds the cursor at point (screen
		* pixels, the same space as MousePoint()), e.g. the centre of the
		* window. Calling it again moves the lock to the new point.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LockCursor() と同じだが、point（画面ピクセル。MousePoint() と
		* 同じ座標）にカーソルを留める。例: ウィンドウの中央。もう一度
		* 呼ぶと、固定する位置が新しい point に移る。
		*/
		static void LockCursor(Vector2 point);

		/**
		* [EN]
		* Releases the lock set by LockCursor(); the cursor stays where it is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LockCursor() による固定を解く。カーソルはその場に残る。
		*/
		static void UnlockCursor();

		/**
		* [EN]
		* Hides the cursor while it is over the game's window. Position,
		* movement and buttons are still reported as usual. Stopping play in
		* the editor always shows it again.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームのウィンドウ上にある間、カーソルを見えなくする。位置・
		* 移動量・ボタンはこれまで通り返す。エディタでプレイを止めると必ず
		* 再表示される。
		*/
		static void HideCursor();

		/**
		* [EN]
		* Shows the cursor again after HideCursor().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* HideCursor() で隠したカーソルを再び表示する。
		*/
		static void RevealCursor();

	private:
		Input() = delete;
	};
}
