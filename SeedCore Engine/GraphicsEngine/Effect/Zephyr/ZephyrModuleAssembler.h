#pragma once
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class ZephyrModuleAssembler :public NonTransferable
	{
	public:
		ZephyrModuleAssembler() = default;
		~ZephyrModuleAssembler() = default;

		NODISCARD String GetOrCreate(const DynamicArray<String>& moduleNames);

	private:
		FlatMap<Uint64, String> generatedCache_;
	};
}
