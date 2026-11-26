#include <FoundationEngine/File/FileDialog.h>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

namespace SeedCore
{
	namespace
	{
		std::wstring MakeFilter(const Wchar* filterName, const Wchar* filterExt)
		{
			std::wstring filter;
			filter += filterName;
			filter.push_back(L'\0');
			filter += filterExt;
			filter.push_back(L'\0');

			filter.push_back(L'\0');
			return filter;
		}
	}

	Bool FileDialog::OpenFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt)
	{
		Wchar fileBuffer[MAX_PATH] = {};

		std::wstring filter = MakeFilter(filterName, filterExt);
		std::wstring initialDirStr = initialDir.wstring();

		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);

		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFilter = filter.c_str();
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDirStr.c_str();

		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (!GetOpenFileNameW(&ofn))
		{
			return false;
		}

		outPath = fileBuffer;
		return true;
	}

	Bool FileDialog::SaveFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt, const Wchar* defaultExt, const Wchar* initialFileName)
	{
		Wchar fileBuffer[MAX_PATH] = {};

		if (initialFileName != nullptr && initialFileName[0] != L'\0')
		{
			wcsncpy_s(fileBuffer, initialFileName, _TRUNCATE);
		}

		std::wstring filter = MakeFilter(filterName, filterExt);
		std::wstring initialDirStr = initialDir.wstring();

		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);

		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFilter = filter.c_str();
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDirStr.c_str();

		ofn.lpstrDefExt = defaultExt;

		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		if (!GetSaveFileNameW(&ofn))
		{
			return false;
		}

		outPath = fileBuffer;
		return true;
	}
}
