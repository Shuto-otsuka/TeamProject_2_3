#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>

namespace SeedCore
{
	const DynamicArray<ZephyrModuleDescriptor>& ZephyrModuleRegistry::Modules()
	{
		static const DynamicArray<ZephyrModuleDescriptor> modules =
		{
			ZephyrModuleDescriptor{ String("Gravity"), String("GraphicsEngine/Effect/Zephyr/Module/GravityModule.hlsli"), String("GravityModule"), { ZephyrModuleField{ String("gravity_"), String("xyz") } }, false, true},
			ZephyrModuleDescriptor{ String("Drag"), String("GraphicsEngine/Effect/Zephyr/Module/DragModule.hlsli"), String("DragModule"), { ZephyrModuleField{ String("drag_"), String("x") } }, false, true },
		};
		return modules;
	}

	const ZephyrModuleDescriptor* ZephyrModuleRegistry::Find(const String& name)
	{
		const DynamicArray<ZephyrModuleDescriptor>& modules = Modules();
		auto found = std::ranges::find_if(modules, [&name](const ZephyrModuleDescriptor& module_) { return module_.name_ == name; });
		if (found == modules.end())
		{
			return nullptr;
		}
		return &(*found);
	}
}
