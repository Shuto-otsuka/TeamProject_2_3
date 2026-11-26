#include <FoundationEngine/File/FileUtility.h>

namespace SeedCore
{
	std::string FileUtility::LoadFileText(String filePath)
	{
		std::ifstream ifs(filePath.c_str(), std::ios::binary);

		if (!ifs)
		{
			return "";
		}

		return std::string((std::istreambuf_iterator<Char>(ifs)), std::istreambuf_iterator<Char>());
	}

	DynamicArray<Uint8> FileUtility::LoadFileBinary(String filePath)
	{
		std::ifstream ifs(filePath.c_str(), std::ios::binary | std::ios::ate);

		if (!ifs)
		{
			return {};
		}

		Size size = static_cast<Size>(ifs.tellg());
		DynamicArray<Uint8> buffer(size);

		ifs.seekg(0, std::ios::beg);
		ifs.read(reinterpret_cast<Char*>(buffer.data()), size);
		return buffer;
	}
}