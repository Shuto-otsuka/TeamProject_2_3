#pragma once

namespace SeedCore
{
	template<typename T>
	class Revisionable
	{
	public:
		void OnRevision()
		{
			static_cast<T*>(this)->Revision();
		}
	};

	template<typename T>
	concept HasRevision = requires(T & a)
	{
		{ a.OnRevision() } -> std::same_as<void>;
	};
}
