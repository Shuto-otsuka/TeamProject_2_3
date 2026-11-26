#pragma once

namespace SeedCore
{
	template<typename T>
	class Awakeable
	{
	public:
		void OnAwake()
		{
			static_cast<T*>(this)->Awake();
		}
	};

	template<typename T>
	concept HasAwake = requires(T& a)
	{
		{ a.OnAwake() } -> std::same_as<void>;
	};
}
