#pragma once

namespace SeedCore
{
	class SEEDCORE_API WorldTimer
	{
	public:
		void Tick(Float delta);

		Float DeltaTime()const;

		Float TotalTime()const;

	private:
		Float delta_ = 0.0f;

		Double total_ = 0.0;
	};
}
