#pragma once

namespace SeedCore
{
	class SEEDCORE_API NonTransferable
	{
	protected:
		constexpr NonTransferable() = default;

		~NonTransferable() = default;

	public:
		NonTransferable(const NonTransferable&) = delete;
		NonTransferable& operator=(const NonTransferable&) = delete;

		NonTransferable(NonTransferable&&) = delete;
		NonTransferable& operator=(NonTransferable&&) = delete;
	};
}