#pragma once
#include <FoundationEngine/Utility/DynamicArray.h>

namespace SeedCore
{
	struct ZephyrModuleDescriptor
	{
		String name_;
		String includePath_;
		String configStructName_;
		Bool hasSpawn_;
		Bool hasUpdate_;
	};

	class ZephyrModuleRegistry
	{
	public:
		NODISCARD static const DynamicArray<ZephyrModuleDescriptor>& GetModules();

		NODISCARD static const ZephyrModuleDescriptor* Find(const String& name);
	};
}
