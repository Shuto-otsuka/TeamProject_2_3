#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* The World service through which gameplay code asks the application
	* for things only the host that owns the main loop can do, such as
	* quitting. It only records each request; what a request means is
	* decided by the host (the runtime closes the application, while the
	* editor stops Play mode instead). Each request is a RequestX / ConsumeX
	* pair: gameplay code calls RequestX at any time, and the host calls
	* ConsumeX once per frame, which reports and clears it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メインループを所有するホストにしかできないこと（終了など）を、
	* ゲームプレイのコードがアプリケーションへ求めるための World の
	* サービス。要求を記録するだけで、要求が何を意味するかはホストが決める
	* （ランタイムではアプリケーションを閉じ、エディタでは代わりに Play
	* モードを止める）。要求はどれも RequestX / ConsumeX の組で、ゲーム
	* プレイのコードはいつでも RequestX を呼び、ホストは毎フレーム1回
	* ConsumeX を呼んで、要求を受け取ると同時に取り下げる。
	*/
	class SEEDCORE_API Application :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Records a request to quit. Several calls before the host consumes
		* it count as one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 終了の要求を記録する。ホストが受け取る前に何度呼んでも、1回分に
		* まとまる。
		*/
		void RequestQuit();

		/**
		* [EN]
		* Returns whether a quit has been requested since the last call, and
		* clears the request in the same step, so each request is seen
		* exactly once. Called once per frame by the host.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降に終了が要求されたかどうかを返し、同時に要求を
		* 取り下げる。これにより各要求はちょうど1回だけ受け取られる。ホスト
		* が毎フレーム1回呼ぶ。
		*/
		NODISCARD Bool ConsumeQuit();

	private:
		/// [EN] Whether RequestQuit has been called and not yet seen by ConsumeQuit.
		/// [JP] RequestQuit が呼ばれ、まだ ConsumeQuit に受け取られていないかどうか。
		Bool quitRequested_ = false;
	};
}
