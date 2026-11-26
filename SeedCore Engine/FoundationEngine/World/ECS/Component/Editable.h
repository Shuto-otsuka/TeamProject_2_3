#pragma once

namespace SeedCore
{
	template<typename T>
	class Editable
	{
	public:
		void OnEditorTick(Float elapsedTime)
		{
			static_cast<T*>(this)->EditorTick(elapsedTime);
		}
	};

	template<typename T>
	concept HasEditorTick = requires(T & a, Float elapsedTime)
	{
		{ a.OnEditorTick(elapsedTime) } -> std::same_as<void>;
	};
}