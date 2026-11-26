#include <Editor/Editor/Build/VisualStudioAutomation.h>

namespace SeedCore
{
	Bool VisualStudioAutomation::InvokeMember(IDispatch* dispatch, const Wchar* name, WORD flags, VARIANT* arg, VARIANT* result)
	{
		if (!dispatch)
		{
			return false;
		}

		Wchar* nameNonConst = const_cast<Wchar*>(name);
		DISPID dispID = 0;
		if (FAILED(dispatch->GetIDsOfNames(IID_NULL, &nameNonConst, 1, LOCALE_USER_DEFAULT, &dispID)))
		{
			return false;
		}

		DISPPARAMS params{};
		DISPID putDispID = DISPID_PROPERTYPUT;
		if (arg)
		{
			params.rgvarg = arg;
			params.cArgs = 1;
			if (flags == DISPATCH_PROPERTYPUT)
			{
				params.rgdispidNamedArgs = &putDispID;
				params.cNamedArgs = 1;
			}
		}

		EXCEPINFO exceptionInfo{};
		HRESULT hr = dispatch->Invoke(dispID, IID_NULL, LOCALE_USER_DEFAULT, flags, &params, result, &exceptionInfo, nullptr);
		return SUCCEEDED(hr);
	}

	IDispatch* VisualStudioAutomation::GetDispatchProperty(IDispatch* dispatch, const Wchar* name)
	{
		VARIANT result;
		VariantInit(&result);
		if (!InvokeMember(dispatch, name, DISPATCH_PROPERTYGET, nullptr, &result))
		{
			VariantClear(&result);
			return nullptr;
		}

		IDispatch* propertyDispatch = (result.vt == VT_DISPATCH) ? result.pdispVal : nullptr;
		if (propertyDispatch)
		{
			propertyDispatch->AddRef();
		}
		VariantClear(&result);
		return propertyDispatch;
	}

	std::wstring VisualStudioAutomation::GetStringProperty(IDispatch* dispatch, const Wchar* name)
	{
		VARIANT result;
		VariantInit(&result);
		if (!InvokeMember(dispatch, name, DISPATCH_PROPERTYGET, nullptr, &result))
		{
			VariantClear(&result);
			return std::wstring();
		}

		std::wstring value = (result.vt == VT_BSTR && result.bstrVal) ? std::wstring(result.bstrVal) : std::wstring();
		VariantClear(&result);
		return value;
	}

	Int VisualStudioAutomation::GetInt32Property(IDispatch* dispatch, const Wchar* name)
	{
		VARIANT result;
		VariantInit(&result);
		if (!InvokeMember(dispatch, name, DISPATCH_PROPERTYGET, nullptr, &result))
		{
			VariantClear(&result);
			return 0;
		}

		if (result.vt != VT_I4)
		{
			if (FAILED(VariantChangeType(&result, &result, 0, VT_I4)))
			{
				VariantClear(&result);
				return 0;
			}
		}

		Int value = result.lVal;
		VariantClear(&result);
		return value;
	}

	IDispatch* VisualStudioAutomation::FindDTEForSolution(const std::filesystem::path& solutionPath)
	{
		std::wstring targetPath = solutionPath.wstring();

		IRunningObjectTable* rot = nullptr;
		if (FAILED(GetRunningObjectTable(0, &rot)))
		{
			return nullptr;
		}

		IEnumMoniker* enumMoniker = nullptr;
		if (FAILED(rot->EnumRunning(&enumMoniker)))
		{
			rot->Release();
			return nullptr;
		}

		IDispatch* matchedDTE = nullptr;

		IMoniker* moniker = nullptr;
		while (!matchedDTE && enumMoniker->Next(1, &moniker, nullptr) == S_OK)
		{
			IBindCtx* bindContext = nullptr;
			if (SUCCEEDED(CreateBindCtx(0, &bindContext)))
			{
				Wchar* displayName = nullptr;
				if (SUCCEEDED(moniker->GetDisplayName(bindContext, nullptr, &displayName)))
				{
					std::wstring monikerName(displayName);
					CoTaskMemFree(displayName);

					if (monikerName.starts_with(L"!VisualStudio.DTE"))
					{
						IUnknown* unknown = nullptr;
						if (SUCCEEDED(rot->GetObject(moniker, &unknown)))
						{
							IDispatch* dte = nullptr;
							if (SUCCEEDED(unknown->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&dte))))
							{
								IDispatch* solution = GetDispatchProperty(dte, L"Solution");
								if (solution)
								{
									std::wstring fullName = GetStringProperty(solution, L"FullName");
									solution->Release();

									if (!fullName.empty() && _wcsicmp(fullName.c_str(), targetPath.c_str()) == 0)
									{
										matchedDTE = dte;
										dte = nullptr;
									}
								}

								if (dte)
								{
									dte->Release();
								}
							}
							unknown->Release();
						}
					}
				}
				bindContext->Release();
			}
			moniker->Release();
		}

		enumMoniker->Release();
		rot->Release();

		return matchedDTE;
	}

	IDispatch* VisualStudioAutomation::FindOrCreateFilter(IDispatch* owner, const std::wstring& name, const std::wstring& onDiskFiltersContent)
	{
		IDispatch* filters = GetDispatchProperty(owner, L"Filters");
		if (!filters)
		{
			return nullptr;
		}

		IDispatch* found = nullptr;
		Int filterCount = GetInt32Property(filters, L"Count");

		for (Int index = 1; !found && index <= filterCount; ++index)
		{
			VARIANT indexArg;
			VariantInit(&indexArg);
			indexArg.vt = VT_I4;
			indexArg.lVal = index;

			VARIANT itemResult;
			VariantInit(&itemResult);
			if (InvokeMember(filters, L"Item", DISPATCH_METHOD, &indexArg, &itemResult) && itemResult.vt == VT_DISPATCH && itemResult.pdispVal)
			{
				std::wstring candidateName = GetStringProperty(itemResult.pdispVal, L"Name");
				if (candidateName == name)
				{
					found = itemResult.pdispVal;
					found->AddRef();
				}
			}
			VariantClear(&itemResult);
		}

		if (!found)
		{
			std::wstring includeNeedle = L"Include=\"" + name + L"\"";
			if (onDiskFiltersContent.contains(includeNeedle))
			{
				filters->Release();
				return nullptr;
			}

			VARIANT nameArg;
			VariantInit(&nameArg);
			nameArg.vt = VT_BSTR;
			nameArg.bstrVal = SysAllocString(name.c_str());

			VARIANT addResult;
			VariantInit(&addResult);
			if (InvokeMember(owner, L"AddFilter", DISPATCH_METHOD, &nameArg, &addResult) && addResult.vt == VT_DISPATCH && addResult.pdispVal)
			{
				found = addResult.pdispVal;
				found->AddRef();
			}
			VariantClear(&addResult);
			VariantClear(&nameArg);
		}

		filters->Release();
		return found;
	}

	IDispatch* VisualStudioAutomation::EnsureFilterChain(IDispatch* vcProject, const std::wstring& relativeDirectory, const std::wstring& onDiskFiltersContent)
	{
		if (relativeDirectory.empty())
		{
			vcProject->AddRef();
			return vcProject;
		}

		IDispatch* current = vcProject;
		Bool ownsCurrent = false;

		Size segmentStart = 0;
		while (segmentStart <= relativeDirectory.size())
		{
			Size separatorIndex = relativeDirectory.find(L'\\', segmentStart);
			std::wstring segment = relativeDirectory.substr(segmentStart, (separatorIndex == std::wstring::npos) ? std::wstring::npos : separatorIndex - segmentStart);

			IDispatch* next = FindOrCreateFilter(current, segment, onDiskFiltersContent);
			if (ownsCurrent)
			{
				current->Release();
			}

			if (!next)
			{
				return nullptr;
			}

			current = next;
			ownsCurrent = true;

			if (separatorIndex == std::wstring::npos)
			{
				break;
			}
			segmentStart = separatorIndex + 1;
		}

		return current;
	}

	Bool VisualStudioAutomation::TryAddFile(const std::filesystem::path& solutionPath, const std::string& projectName, const std::filesystem::path& headerPath, const std::filesystem::path& cppPath)
	{
		Bool comInitializedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

		Bool success = false;

		IDispatch* dte = FindDTEForSolution(solutionPath);
		if (dte)
		{
			IDispatch* solution = GetDispatchProperty(dte, L"Solution");
			if (solution)
			{
				IDispatch* projects = GetDispatchProperty(solution, L"Projects");
				if (projects)
				{
					IDispatch* project = nullptr;
					Int projectCount = GetInt32Property(projects, L"Count");
					std::wstring wideProjectName(projectName.begin(), projectName.end());

					for (Int index = 1; !project && index <= projectCount; ++index)
					{
						VARIANT indexArg;
						VariantInit(&indexArg);
						indexArg.vt = VT_I4;
						indexArg.lVal = index;

						VARIANT itemResult;
						VariantInit(&itemResult);
						if (InvokeMember(projects, L"Item", DISPATCH_METHOD, &indexArg, &itemResult) && itemResult.vt == VT_DISPATCH && itemResult.pdispVal)
						{
							std::wstring candidateName = GetStringProperty(itemResult.pdispVal, L"Name");
							if (candidateName == wideProjectName)
							{
								project = itemResult.pdispVal;
								project->AddRef();
							}
						}
						VariantClear(&itemResult);
					}

					if (project)
					{
						IDispatch* vcProject = GetDispatchProperty(project, L"Object");
						if (vcProject)
						{
							std::filesystem::path projectRoot = solutionPath.parent_path().parent_path();
							std::filesystem::path userProjectRoot = projectRoot / "UserProject";
							std::error_code relativeError;
							std::filesystem::path relativeDirectoryPath = std::filesystem::relative(headerPath.parent_path(), userProjectRoot, relativeError);
							std::wstring relativeDirectory = (!relativeError && relativeDirectoryPath != ".") ? relativeDirectoryPath.wstring() : std::wstring();

							std::wstring onDiskFiltersContent;
							{
								std::ifstream filtersFile(userProjectRoot / "UserProject.Cplusplus.vcxproj.filters", std::ios::binary);
								if (filtersFile)
								{
									std::string rawContent((std::istreambuf_iterator<Char>(filtersFile)), std::istreambuf_iterator<Char>());
									onDiskFiltersContent.assign(rawContent.begin(), rawContent.end());
								}
							}

							IDispatch* leafFilter = EnsureFilterChain(vcProject, relativeDirectory, onDiskFiltersContent);
							if (leafFilter)
							{
								Bool addedHeader = false;
								Bool addedCpp = false;

								for (const std::filesystem::path& path : { headerPath, cppPath })
								{
									VARIANT pathArg;
									VariantInit(&pathArg);
									pathArg.vt = VT_BSTR;
									pathArg.bstrVal = SysAllocString(path.wstring().c_str());

									VARIANT addResult;
									VariantInit(&addResult);
									Bool added = InvokeMember(leafFilter, L"AddFile", DISPATCH_METHOD, &pathArg, &addResult);
									VariantClear(&addResult);
									VariantClear(&pathArg);

									if (path == headerPath)
									{
										addedHeader = added;
									}
									else
									{
										addedCpp = added;
									}
								}

								success = addedHeader && addedCpp;
								leafFilter->Release();
							}
							vcProject->Release();
						}

						if (success)
						{
							VARIANT emptyPath;
							VariantInit(&emptyPath);
							emptyPath.vt = VT_BSTR;
							emptyPath.bstrVal = SysAllocString(L"");
							VARIANT saveResult;
							VariantInit(&saveResult);
							Bool saved = InvokeMember(project, L"Save", DISPATCH_METHOD, &emptyPath, &saveResult);
							(void)saved;
							VariantClear(&saveResult);
							VariantClear(&emptyPath);
						}

						project->Release();
					}
					projects->Release();
				}
				solution->Release();
			}
			dte->Release();
		}

		if (comInitializedHere)
		{
			CoUninitialize();
		}

		return success;
	}

	Bool VisualStudioAutomation::TryOpenFile(const std::filesystem::path& solutionPath, const std::filesystem::path& filePath)
	{
		Bool comInitializedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

		Bool success = false;

		IDispatch* dte = FindDTEForSolution(solutionPath);
		if (dte)
		{
			IDispatch* itemOperations = GetDispatchProperty(dte, L"ItemOperations");
			if (itemOperations)
			{
				VARIANT pathArg;
				VariantInit(&pathArg);
				pathArg.vt = VT_BSTR;
				pathArg.bstrVal = SysAllocString(filePath.wstring().c_str());

				VARIANT openResult;
				VariantInit(&openResult);
				success = InvokeMember(itemOperations, L"OpenFile", DISPATCH_METHOD, &pathArg, &openResult);
				VariantClear(&openResult);
				VariantClear(&pathArg);

				itemOperations->Release();
			}

			IDispatch* mainWindow = success ? GetDispatchProperty(dte, L"MainWindow") : nullptr;
			if (mainWindow)
			{
				AllowSetForegroundWindow(ASFW_ANY);

				VARIANT activateResult;
				VariantInit(&activateResult);
				Bool activated = InvokeMember(mainWindow, L"Activate", DISPATCH_METHOD, nullptr, &activateResult);
				(void)activated;
				VariantClear(&activateResult);

				mainWindow->Release();
			}

			dte->Release();
		}

		if (comInitializedHere)
		{
			CoUninitialize();
		}

		return success;
	}
}
