#include <Editor/Editor/Build/HotReload.h>
#include <FoundationEngine/File/FileDirectory.h>
#include <FoundationEngine/File/FileLocator.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/Plugin/PluginHost.h>
#include <FoundationEngine/Plugin/PluginModule.h>
#include <FoundationEngine/Bridge/CsharpHost.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	namespace
	{
		constexpr Uint64 SourceStableWindowMilliseconds = 150;

		constexpr Uint64 SourceScanIntervalMilliseconds = 150;
	}

	HotReload::~HotReload()
	{
		for (BuildTask* task : { &cplusplusBuild_, &csharpBuild_ })
		{
			if (task->process_)
			{
				CloseHandle(task->process_);
			}
			if (task->outputPipe_)
			{
				CloseHandle(task->outputPipe_);
			}
		}
	}

	Bool HotReload::Initialize(PluginHost& pluginHost, CsharpHost& csharpHost)
	{
		pluginHost_ = &pluginHost;
		userProjectPlugin_ = pluginHost.Find("UserProject.Cplusplus");
		csharpHost_ = &csharpHost;

		std::filesystem::path csharpAssembly = FileDirectory::ExecutableDirectory().parent_path() / "Csharp" / "UserProject.Csharp.dll";
		Uint64 csharpSourceWriteTime = ScanSourceLastWriteTime(true);
		if (GetLastWriteTime(csharpAssembly) >= csharpSourceWriteTime)
		{
			csharpBuild_.lastTriggeredWriteTime_ = csharpSourceWriteTime;
		}
		return true;
	}

	std::filesystem::path HotReload::UserProjectSourceDirectory()
	{
		return FileLocator::ProjectRoot();
	}

	std::filesystem::path HotReload::UserProjectVcxprojPath()
	{
		return UserProjectSourceDirectory() / "UserProject.Cplusplus.vcxproj";
	}

	std::filesystem::path HotReload::UserProjectCsprojPath()
	{
		return UserProjectSourceDirectory() / "UserProject.Csharp.csproj";
	}

	Uint64 HotReload::GetLastWriteTime(const std::filesystem::path& path)
	{
		WIN32_FILE_ATTRIBUTE_DATA attributeData{};
		if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attributeData))
		{
			return 0;
		}

		ULARGE_INTEGER writeTime{};
		writeTime.LowPart = attributeData.ftLastWriteTime.dwLowDateTime;
		writeTime.HighPart = attributeData.ftLastWriteTime.dwHighDateTime;
		return writeTime.QuadPart;
	}

	Uint64 HotReload::ScanSourceLastWriteTime(Bool csharp)
	{
		std::error_code errorCode;
		std::filesystem::path sourceDirectory = UserProjectSourceDirectory();

		Uint64 newest = 0;

		auto iterator = std::filesystem::recursive_directory_iterator(sourceDirectory, std::filesystem::directory_options::skip_permission_denied, errorCode);
		if (errorCode)
		{
			return 0;
		}

		for (const std::filesystem::directory_entry& entry : iterator)
		{
			if (!entry.is_regular_file(errorCode))
			{
				continue;
			}

			std::filesystem::path extension = entry.path().extension();
			if (csharp)
			{
				if (extension != L".cs")
				{
					continue;
				}
			}
			else if (extension != L".cpp" && extension != L".h" && extension != L".hpp")
			{
				continue;
			}

			std::wstring fileName = entry.path().filename().wstring();
			if (fileName.ends_with(L".generated.cpp") || fileName.ends_with(L".generated.cs"))
			{
				continue;
			}

			Uint64 writeTime = GetLastWriteTime(entry.path());
			if (writeTime > newest)
			{
				newest = writeTime;
			}
		}

		return newest;
	}

	std::filesystem::path HotReload::FindMSBuild()
	{
		std::filesystem::path vswhere = LR"(C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe)";
		if (!std::filesystem::exists(vswhere))
		{
			return {};
		}

		std::wstring commandLine = std::format(L"\"{}\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe", vswhere.wstring());

		SECURITY_ATTRIBUTES securityAttributes{};
		securityAttributes.nLength = sizeof(securityAttributes);
		securityAttributes.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
		{
			return {};
		}
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;

		PROCESS_INFORMATION processInfo{};

		DynamicArray<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
		mutableCommandLine.push_back(L'\0');

		Bool created = CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		CloseHandle(writePipe);

		std::filesystem::path result;

		if (created)
		{
			std::string output;
			Char readBuffer[512]{};
			DWORD bytesRead = 0;
			while (ReadFile(readPipe, readBuffer, sizeof(readBuffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
			{
				output.append(readBuffer, bytesRead);
			}

			WaitForSingleObject(processInfo.hProcess, INFINITE);
			CloseHandle(processInfo.hProcess);
			CloseHandle(processInfo.hThread);

			while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
			{
				output.pop_back();
			}

			if (!output.empty())
			{
				result = std::filesystem::path(output);
			}
		}

		CloseHandle(readPipe);

		return result;
	}

	void HotReload::TriggerCplusplusBuild()
	{
		if (!msbuildResolved_)
		{
			msbuildPath_ = FindMSBuild();
			msbuildResolved_ = true;

			if (msbuildPath_.empty())
			{
				SC_LOG_WARNING("HotReload: MSBuild.exe が見つかりません。UserProject の自動ビルドは無効になります。");
			}
		}

		if (msbuildPath_.empty())
		{
			return;
		}

#if SC_DEVELOP
		const Wchar* configuration = L"Develop";
#elif SC_DEBUG
		const Wchar* configuration = L"Debug";
#else
		const Wchar* configuration = L"Release";
#endif

		std::filesystem::path solutionDir = UserProjectSourceDirectory().parent_path() / L"Runtime\\";

		std::wstring solutionDirArgument = solutionDir.wstring();
		if (!solutionDirArgument.empty() && solutionDirArgument.back() == L'\\')
		{
			solutionDirArgument.push_back(L'\\');
		}

		std::wstring hotReloadPdbName = std::format(L"UserProject.Cplusplus_HotReload_{}", GetTickCount64());

		std::wstring commandLine = std::format(L"\"{}\" \"{}\" /nologo /verbosity:minimal /p:Configuration={} /p:Platform=x64 /p:BuildProjectReferences=false /p:HotReloadPdbName={} /p:SolutionDir=\"{}\"", msbuildPath_.wstring(), UserProjectVcxprojPath().wstring(), configuration, hotReloadPdbName, solutionDirArgument);

		Launch(cplusplusBuild_, commandLine, "UserProject");
	}

	void HotReload::TriggerCsharpBuild()
	{
#if SC_DEVELOP
		const Wchar* configuration = L"Develop";
#elif SC_DEBUG
		const Wchar* configuration = L"Debug";
#else
		const Wchar* configuration = L"Release";
#endif

		std::wstring commandLine = std::format(L"dotnet build \"{}\" --no-dependencies -c {} -nologo -v:q", UserProjectCsprojPath().wstring(), configuration);

		Launch(csharpBuild_, commandLine, "UserProject (C#)");
	}

	Bool HotReload::Launch(BuildTask& task, std::wstring commandLine, const Char* label)
	{
		SECURITY_ATTRIBUTES securityAttributes{};
		securityAttributes.nLength = sizeof(securityAttributes);
		securityAttributes.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
		{
			SC_LOG_WARNING("HotReload: {} の自動ビルド起動に失敗しました(パイプ作成失敗)。", label);
			return false;
		}
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;
		PROCESS_INFORMATION processInfo{};

		DynamicArray<Wchar> mutableCommandLine(commandLine.begin(), commandLine.end());
		mutableCommandLine.push_back(L'\0');

		Bool created = CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		CloseHandle(writePipe);

		if (!created)
		{
			SC_LOG_WARNING("HotReload: {} の自動ビルド起動に失敗しました。", label);
			CloseHandle(readPipe);
			return false;
		}

		CloseHandle(processInfo.hThread);
		task.process_ = processInfo.hProcess;
		task.outputPipe_ = readPipe;
		task.output_.clear();

		SC_LOG_NOTICE("HotReload: {} の自動ビルドを開始しました。", label);
		return true;
	}

	void HotReload::Drain(BuildTask& task)
	{
		if (!task.outputPipe_)
		{
			return;
		}

		DWORD bytesAvailable = 0;
		while (PeekNamedPipe(task.outputPipe_, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0)
		{
			Char buffer[512]{};
			DWORD bytesRead = 0;
			if (!ReadFile(task.outputPipe_, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) || bytesRead == 0)
			{
				break;
			}
			task.output_.append(buffer, bytesRead);
		}
	}

	void HotReload::Poll(BuildTask& task, const Char* label)
	{
		if (!task.process_)
		{
			return;
		}

		Drain(task);

		if (WaitForSingleObject(task.process_, 0) != WAIT_OBJECT_0)
		{
			return;
		}

		Drain(task);

		DWORD exitCode = 0;
		GetExitCodeProcess(task.process_, &exitCode);
		CloseHandle(task.process_);
		task.process_ = nullptr;

		CloseHandle(task.outputPipe_);
		task.outputPipe_ = nullptr;

		if (exitCode == 0)
		{
			SC_LOG_NOTICE("HotReload: {} の自動ビルドが完了しました。", label);

			task.reloadRequested_ = true;
		}
		else
		{
			SC_LOG_WARNING("HotReload: {} の自動ビルドが失敗しました(終了コード {})。\n{}", label, exitCode, task.output_);
		}

		task.output_.clear();
	}

	Bool HotReload::Settled(BuildTask& task, Uint64 writeTime, Uint64 nowTick)
	{
		if (writeTime == 0 || writeTime == task.lastTriggeredWriteTime_)
		{
			return false;
		}

		if (writeTime != task.pendingWriteTime_)
		{
			task.pendingWriteTime_ = writeTime;
			task.pendingStableSinceTick_ = nowTick;
			return false;
		}

		if (nowTick - task.pendingStableSinceTick_ < SourceStableWindowMilliseconds)
		{
			return false;
		}

		task.lastTriggeredWriteTime_ = writeTime;
		return true;
	}

	void HotReload::Tick(World& world)
	{
		Poll(cplusplusBuild_, "UserProject");
		Poll(csharpBuild_, "UserProject (C#)");

		Uint64 nowTick = GetTickCount64();

		if (!cplusplusBuild_.process_ && nowTick - cplusplusBuild_.lastScanTick_ >= SourceScanIntervalMilliseconds)
		{
			cplusplusBuild_.lastScanTick_ = nowTick;
			if (Settled(cplusplusBuild_, ScanSourceLastWriteTime(false), nowTick))
			{
				TriggerCplusplusBuild();
			}
		}

		if (csharpHost_ && !csharpBuild_.process_ && nowTick - csharpBuild_.lastScanTick_ >= SourceScanIntervalMilliseconds)
		{
			csharpBuild_.lastScanTick_ = nowTick;
			if (Settled(csharpBuild_, ScanSourceLastWriteTime(true), nowTick))
			{
				TriggerCsharpBuild();
			}
		}

		if (cplusplusBuild_.reloadRequested_ && world.GetService<GameTimer>()->Playing())
		{
			if (!cplusplusReloadDeferred_)
			{
				SC_LOG_NOTICE("HotReload: 再生中のため、UserProject のリロードは再生を止めたときに行います。");
				cplusplusReloadDeferred_ = true;
			}
		}
		else if (cplusplusBuild_.reloadRequested_)
		{
			cplusplusBuild_.reloadRequested_ = false;
			cplusplusReloadDeferred_ = false;

			if (!userProjectPlugin_ && pluginHost_)
			{
				userProjectPlugin_ = pluginHost_->Find("UserProject.Cplusplus");
			}

			if (pluginHost_ && userProjectPlugin_)
			{
				pluginHost_->ReloadModule(world, *userProjectPlugin_);
			}
		}

		if (csharpBuild_.reloadRequested_)
		{
			if (world.GetService<GameTimer>()->Playing())
			{
				if (!csharpReloadDeferred_)
				{
					SC_LOG_NOTICE("HotReload: 再生中のため、C# スクリプトのリロードは再生を止めたときに行います。");
					csharpReloadDeferred_ = true;
				}
			}
			else
			{
				csharpBuild_.reloadRequested_ = false;
				csharpReloadDeferred_ = false;
				csharpHost_->Reload(world);
			}
		}
	}
}
