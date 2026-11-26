#pragma once

namespace SeedCore
{
	template<typename T>
	class LateTickable
	{
	public:
		void OnLateTick(Float elapsedTime)
		{
			static_cast<T*>(this)->LateTick(elapsedTime);
		}
	};

	template<typename T>
	concept HasLateTick = requires(T& a, Float elapsedTime)
	{
		{ a.OnLateTick(elapsedTime) } -> std::same_as<void>;
	};
}
