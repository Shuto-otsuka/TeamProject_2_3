#pragma once
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class SEEDCORE_API PayloadRegistry
	{
	public:
		using PayloadFunc = std::function<void(void*, DynamicArray<FieldInfo>&)>;

		static FlatMap<String, PayloadFunc>& GetRegistry();

		static void Register(String name, PayloadFunc function);
	};
}
