#pragma once

namespace SeedCore
{
	class VisualStudioAutomation
	{
	public:
		NODISCARD static Bool TryAddFile(const std::filesystem::path& solutionPath, const std::string& projectName, const std::filesystem::path& headerPath, const std::filesystem::path& cppPath);

		NODISCARD static Bool TryOpenFile(const std::filesystem::path& solutionPath, const std::filesystem::path& filePath);

	private:
		NODISCARD static Bool InvokeMember(IDispatch* dispatch, const Wchar* name, WORD flags, VARIANT* arg, VARIANT* result);

		NODISCARD static IDispatch* GetDispatchProperty(IDispatch* dispatch, const Wchar* name);

		NODISCARD static std::wstring GetStringProperty(IDispatch* dispatch, const Wchar* name);

		NODISCARD static Int GetInt32Property(IDispatch* dispatch, const Wchar* name);

		NODISCARD static IDispatch* FindDTEForSolution(const std::filesystem::path& solutionPath);

		NODISCARD static IDispatch* FindOrCreateFilter(IDispatch* owner, const std::wstring& name, const std::wstring& onDiskFiltersContent);

		NODISCARD static IDispatch* EnsureFilterChain(IDispatch* vcProject, const std::wstring& relativeDirectory, const std::wstring& onDiskFiltersContent);
	};
}
