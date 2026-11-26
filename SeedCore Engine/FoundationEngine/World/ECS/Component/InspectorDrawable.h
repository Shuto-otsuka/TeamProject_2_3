#pragma once

namespace SeedCore
{
	template<typename T>
	class InspectorDrawable
	{
	public:
		void OnInspectorGUI()
		{
			static_cast<T*>(this)->InspectorGUI();
		}
	};

	template<typename T>
	concept HasInspectorGUI = requires(T& a)
	{
		{ a.OnInspectorGUI() } -> std::same_as<void>;
	};
}
