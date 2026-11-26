#pragma once

namespace SeedCore
{
	template<typename T>
	class Tickable
	{
	public:
		void OnTick(Float elapsedTime)
		{
			static_cast<T*>(this)->Tick(elapsedTime);
		}
	};

	template<typename T>
	concept HasTick = requires(T& a, Float elapsedTime)
	{
		{ a.OnTick(elapsedTime) } -> std::same_as<void>;
	};
}
