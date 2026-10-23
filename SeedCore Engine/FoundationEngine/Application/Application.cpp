#include <FoundationEngine/Application/Application.h>

namespace SeedCore
{
	/**
	* [EN]
	* Records a request to quit. Several calls before the host consumes it
	* count as one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 終了の要求を記録する。ホストが受け取る前に何度呼んでも、1回分に
	* まとまる。
	*/
	void Application::RequestQuit()
	{
		quitRequested_ = true;
	}

	/**
	* [EN]
	* Returns whether a quit has been requested since the last call, and
	* clears the request in the same step, so each request is seen exactly
	* once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の呼び出し以降に終了が要求されたかどうかを返し、同時に要求を
	* 取り下げる。これにより各要求はちょうど1回だけ受け取られる。
	*/
	NODISCARD Bool Application::ConsumeQuit()
	{
		/// [EN] The request is read and cleared together, so a request made after this call waits for the next one.
		/// [JP] 要求の読み取りと取り下げを一緒に行うので、この呼び出しの後に出た要求は次の呼び出しで受け取られる。
		Bool requested = quitRequested_;
		quitRequested_ = false;
		return requested;
	}
}
