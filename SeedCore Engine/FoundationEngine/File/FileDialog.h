#pragma once

namespace SeedCore
{
	class SEEDCORE_API FileDialog
	{
	public:
		static Bool OpenFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt);

		static Bool SaveFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt, const Wchar* defaultExt, const Wchar* initialFileName = nullptr);
	};
}
