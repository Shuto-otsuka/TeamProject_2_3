#pragma once

namespace SeedCore
{
	template<typename T>
	class Detachable
	{
	public:
		void OnDetach()
		{
			static_cast<T*>(this)->Detach();
		}
	};

	template<typename T>
	concept HasDetach = requires(T & a)
	{
		{ a.OnDetach() } -> std::same_as<void>;
	};
}
