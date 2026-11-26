#pragma once

namespace SeedCore
{
	class SEEDCORE_API FileUtility
	{
	public:
		static std::string LoadFileText(String filePath);

		static DynamicArray<Uint8> LoadFileBinary(String filePath);
	};
}