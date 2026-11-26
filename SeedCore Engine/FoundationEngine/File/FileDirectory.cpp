#include <FoundationEngine/File/FileDirectory.h>

namespace SeedCore
{
	std::filesystem::path FileDirectory::ExecutableDirectory()
	{
		Wchar buffer[MAX_PATH]{};
		GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		return std::filesystem::path(buffer).parent_path();
	}
}