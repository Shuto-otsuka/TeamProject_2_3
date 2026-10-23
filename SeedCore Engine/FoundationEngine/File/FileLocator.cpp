#include <FoundationEngine/File/FileLocator.h>
#include <FoundationEngine/File/FileDirectory.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	std::filesystem::path FileLocator::EngineRoot()
	{
		std::error_code errorCode;
		std::filesystem::path directory = FileDirectory::ExecutableDirectory();
		while (!std::filesystem::is_directory(directory / "CompliedShaderObject", errorCode))
		{
			if (directory == directory.parent_path())
			{
				return std::filesystem::current_path().parent_path();
			}
			directory = directory.parent_path();
		}
		return directory;
	}

	std::filesystem::path FileLocator::ProjectRoot()
	{
		return EngineRoot() / "UserProject";
	}
}