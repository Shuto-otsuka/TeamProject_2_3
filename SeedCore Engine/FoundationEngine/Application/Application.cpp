#include <FoundationEngine/Application/Application.h>

namespace SeedCore
{
	void Application::RequestQuit()
	{
		quitRequested_ = true;
	}

	NODISCARD Bool Application::ConsumeQuit()
	{
		Bool requested = quitRequested_;
		quitRequested_ = false;
		return requested;
	}
}
