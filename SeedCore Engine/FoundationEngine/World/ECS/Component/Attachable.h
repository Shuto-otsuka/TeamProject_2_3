#pragma once

namespace SeedCore
{
	template<typename T>
	class Attachable
	{
	public:
		void OnAttach()
		{
			static_cast<T*>(this)->Attach();
		}
	};

	template<typename T>
	concept HasAttach = requires(T & a)
	{
		{ a.OnAttach() } -> std::same_as<void>;
	};
}
