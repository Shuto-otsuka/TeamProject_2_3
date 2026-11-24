#pragma once
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	struct EffectEmitter;

	struct ZephyrParameterRow
	{
		Uint32 node_ = 0;

		String input_;

		Vector4 default_ = Vector4::Zero;
	};

	struct ZephyrAssembly
	{
		String filePath_;

		DynamicArray<ZephyrParameterRow> rows_;
	};

	class ZephyrModuleAssembler :public NonTransferable
	{
	public:
		ZephyrModuleAssembler() = default;
		~ZephyrModuleAssembler() = default;

		NODISCARD ZephyrAssembly Assemble(const EffectEmitter& emitter);

	private:
		FlatMap<Uint64, ZephyrAssembly> generatedCache_;
	};
}
