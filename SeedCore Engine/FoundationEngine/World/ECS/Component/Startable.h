#pragma once

namespace SeedCore
{
	template<typename T>
	class Startable
	{
	public:
		void OnStart()
		{
			static_cast<T*>(this)->Start();
		}
	};

	template<typename T>
	concept HasStart = requires(T& a)
	{
		{ a.OnStart() } -> std::same_as<void>;
	};
}
