#pragma once

namespace SeedCore
{
	template<typename T>
	class Destroyable
	{
	public:
		void OnDestroy()
		{
			static_cast<T*>(this)->Destroy();
		}
	};

	template<typename T>
	concept HasDestroy = requires(T& a)
	{
		{ a.OnDestroy() } -> std::same_as<void>;
	};
}
