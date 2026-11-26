#pragma once

namespace SeedCore
{
	template<typename T>
	class FixedTickable
	{
	public:
		void OnFixedTick(Float elapsedTime)
		{
			static_cast<T*>(this)->FixedTick(elapsedTime);
		}
	};

	template<typename T>
	concept HasFixedTick = requires(T& a, Float elapsedTime)
	{
		{ a.OnFixedTick(elapsedTime) } -> std::same_as<void>;
	};
}
