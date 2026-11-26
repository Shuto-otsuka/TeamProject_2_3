#pragma once

namespace SeedCore
{
	class SEEDCORE_API Application :public NonTransferable
	{
	public:
		void RequestQuit();

		NODISCARD Bool ConsumeQuit();

	private:
		Bool quitRequested_ = false;
	};
}
