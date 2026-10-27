#pragma once

namespace SeedCore
{
	class SEEDCORE_API AudioByteStream final
	{
	public:
		static void Install();

		static void Uninstall();

	private:
		class Impl;
	};
}
