#pragma once

namespace SeedCore
{
	class SEEDCORE_API Sha256
	{
	public:
		static DynamicArray<Byte> Hash(const Byte* data, Size size);

		static DynamicArray<Byte> Hash(const DynamicArray<Byte>& data);

		static DynamicArray<Byte> Hash(const std::filesystem::path& path);

	private:
		static void ProcessBlock(Uint32 state[8], const Byte block[64]);
	};
}
