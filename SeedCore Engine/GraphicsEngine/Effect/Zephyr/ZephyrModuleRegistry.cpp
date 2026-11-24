#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrNodeRegistry.h>

namespace SeedCore
{
	const DynamicArray<ZephyrModuleDescriptor>& ZephyrModuleRegistry::Modules()
	{
		static const DynamicArray<ZephyrModuleDescriptor> modules = []()
			{
				DynamicArray<ZephyrModuleDescriptor> table;
				for (const ZephyrNodeDefinition& node : ZephyrNodeRegistry::Nodes())
				{
					if (node.include_.str().empty() || node.outputs_.empty())
					{
						continue;
					}
					ZephyrModuleDescriptor& module = table.emplace_back();
					module.name_ = node.type_;
					module.include_ = node.include_;
					module.output_ = node.outputs_[0].type_;
					for (const ZephyrNodeInput& input : node.inputs_)
					{
						module.inputs_.push_back(ZephyrModuleInput{ input.name_, input.type_ });
					}
				}
				return table;
			}();
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
