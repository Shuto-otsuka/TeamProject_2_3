#pragma once

namespace SeedCore
{
	struct ZephyrModuleField
	{
		String name_;
		String swizzle_;
	};

	struct ZephyrModuleDescriptor
	{
		String name_;
		String includePath_;
		String configStructName_;
		DynamicArray<ZephyrModuleField> fields_;
		Bool hasSpawn_;
		Bool hasSimulate_;
	};

	class ZephyrModuleRegistry
	{
	public:
		NODISCARD static const DynamicArray<ZephyrModuleDescriptor>& Modules();

		NODISCARD static const ZephyrModuleDescriptor* Find(const String& name);
	};
}
