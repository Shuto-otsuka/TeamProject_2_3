#include <Editor/Editor/Build/RuntimeBuilder.h>

namespace SeedCore
{
	namespace
	{
		const DynamicArray<std::string> projectNames =
		{
			"FoundationEngine.Cplusplus.vcxproj",
			"AIEngine.Cplusplus.vcxproj",
			"PhysicsEngine.Cplusplus.vcxproj",
			"AudioEngine.Cplusplus.vcxproj",
			"GraphicsEngine.Cplusplus.vcxproj",
			"SeedCore.Cplusplus.vcxproj",
			"UserProject.Cplusplus.vcxproj",
			"Runtime.vcxproj",
		};
	}


	RuntimeBuilder::~RuntimeBuilder()
	{
		if (thread_.joinable())
		{
			thread_.join();
		}
	}

	void RuntimeBuilder::BuildAsync(const std::filesystem::path& projectRoot)
	{
		if (building_)
		{
			return;
		}

		if (thread_.joinable())
		{
			thread_.join();
		}

		stage_ = RuntimeBuildStage::Build;
		progress_ = 0.0f;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			statusText_.clear();
			detailText_.clear();
		}

		building_ = true;

		thread_ = std::thread([this, projectRoot]()
		{
			std::string log;
			Bool success = Build(projectRoot, log);

			{
				std::lock_guard<std::mutex> lock(resultMutex_);
				lastSuccess_ = success;
				lastLog_ = std::move(log);
				hasResult_ = true;
			}

			building_ = false;
		});
	}

	Bool RuntimeBuilder::IsBuilding()const
	{
		return building_;
	}

	RuntimeBuildStage RuntimeBuilder::GetStage()const
	{
		return stage_;
	}

	Float RuntimeBuilder::GetProgress()const
	{
		return progress_;
	}

	std::string RuntimeBuilder::GetStatusText()const
	{
		std::lock_guard<std::mutex> lock(statusMutex_);
		return statusText_;
	}

	std::string RuntimeBuilder::GetDetailText()const
	{
		std::lock_guard<std::mutex> lock(statusMutex_);
		return detailText_;
	}

	Bool RuntimeBuilder::ConsumeResult(Bool& outSuccess, std::string& outLog)
	{
		std::lock_guard<std::mutex> lock(resultMutex_);
		if (!hasResult_)
		{
			return false;
		}

		outSuccess = lastSuccess_;
		outLog = std::move(lastLog_);
		hasResult_ = false;
		return true;
	}

	Bool RuntimeBuilder::RunProcessCaptureOutput(const std::wstring& commandLine, std::string& outResult, const std::function<void(const std::string&)>& onChunk)
	{
		SECURITY_ATTRIBUTES securityAttributes{};
		securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		securityAttributes.bInheritHandle = TRUE;

		HANDLE readHandle = nullptr;
		HANDLE writeHandle = nullptr;
		if (!CreatePipe(&readHandle, &writeHandle, &securityAttributes, 0))
		{
			return false;
		}
		SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(STARTUPINFOW);
		startupInfo.hStdOutput = writeHandle;
		startupInfo.hStdError = writeHandle;
		startupInfo.dwFlags |= STARTF_USESTDHANDLES;

		PROCESS_INFORMATION processInfo{};

		std::wstring mutableCommandLine = commandLine;
		Bool created = CreateProcessW(
			nullptr,
			mutableCommandLine.data(),
			nullptr, nullptr,
			TRUE,
			CREATE_NO_WINDOW,
			nullptr, nullptr,
			&startupInfo,
			&processInfo
		);

		CloseHandle(writeHandle);

		if (!created)
		{
			CloseHandle(readHandle);
			return false;
		}

		constexpr DWORD overallTimeoutMs = 10 * 60 * 1000;
		constexpr DWORD drainGraceMs = 2000;

		auto startTime = std::chrono::steady_clock::now();
		Bool processExited = false;
		auto processExitTime = startTime;

		Char buffer[4096];
		for (;;)
		{
			DWORD available = 0;
			Bool peekSucceeded = PeekNamedPipe(readHandle, nullptr, 0, nullptr, &available, nullptr);
			if (!peekSucceeded)
			{
				break;
			}

			if (available > 0)
			{
				DWORD bytesRead = 0;
				if (ReadFile(readHandle, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
				{
					outResult.append(buffer, bytesRead);
					if (onChunk)
					{
						onChunk(outResult);
					}
				}
				continue;
			}

			if (!processExited && WaitForSingleObject(processInfo.hProcess, 50) == WAIT_OBJECT_0)
			{
				processExited = true;
				processExitTime = std::chrono::steady_clock::now();
			}

			auto now = std::chrono::steady_clock::now();

			if (processExited && std::chrono::duration_cast<std::chrono::milliseconds>(now - processExitTime).count() > drainGraceMs)
			{
				break;
			}

			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() > overallTimeoutMs)
			{
				TerminateProcess(processInfo.hProcess, 1);
				break;
			}
		}

		WaitForSingleObject(processInfo.hProcess, 1000);

		DWORD exitCode = 1;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		if (exitCode == STILL_ACTIVE)
		{
			TerminateProcess(processInfo.hProcess, 1);
			exitCode = 1;
		}

		CloseHandle(readHandle);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return exitCode == 0;
	}

	std::optional<std::filesystem::path> RuntimeBuilder::FindMSBuild(const std::filesystem::path& projectRoot)
	{
		std::filesystem::path vswherePath = projectRoot / "Tools" / "Build" / "vswhere.exe";

		std::wstring commandLine =
			L"\"" + vswherePath.wstring() + L"\" "
			L"-latest -products * -requires Microsoft.Component.MSBuild "
			L"-find MSBuild\\**\\Bin\\MSBuild.exe";

		std::string output;
		if (!RunProcessCaptureOutput(commandLine, output))
		{
			return std::nullopt;
		}

		while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
		{
			output.pop_back();
		}

		if (output.empty())
		{
			return std::nullopt;
		}

		return std::filesystem::path(output);
	}

	Bool RuntimeBuilder::Build(const std::filesystem::path& projectRoot, std::string& outLog)
	{
		auto msbuildPath = FindMSBuild(projectRoot);
		if (!msbuildPath)
		{
			outLog = "MSBuildが見つかりませんでした";
			return false;
		}

		std::filesystem::path runtimeProject = projectRoot / "Runtime" / "Runtime.vcxproj";

		std::wstring solutionDirArgument = (projectRoot / L"Runtime\\").wstring();
		if (!solutionDirArgument.empty() && solutionDirArgument.back() == L'\\')
		{
			solutionDirArgument.push_back(L'\\');
		}

		std::wstring commandLine =
			L"\"" + msbuildPath->wstring() + L"\" "
			L"\"" + runtimeProject.wstring() + L"\" "
			L"-t:Rebuild -p:Configuration=Release -p:Platform=x64 -m "
			L"\"-p:SolutionDir=" + solutionDirArgument + L"\"";

		DynamicArray<Bool> projectCompleted(projectNames.size(), false);
		Uint32 completedProjects = 0;
		Size parsedLength = 0;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			statusText_ = std::format("プロジェクト 0 / {}", projectNames.size());
		}

		auto onChunk = [this, &projectCompleted, &completedProjects, &parsedLength](const std::string& accumulated)
		{
			Size lineEnd = accumulated.find('\n', parsedLength);
			while (lineEnd != std::string::npos)
			{
				std::string_view line(accumulated.data() + parsedLength, lineEnd - parsedLength);
				parsedLength = lineEnd + 1;
				lineEnd = accumulated.find('\n', parsedLength);
				if (!line.empty() && line.back() == '\r')
				{
					line.remove_suffix(1);
				}

				for (Size index = 0; index < projectNames.size(); ++index)
				{
					if (!projectCompleted[index] && line.contains(projectNames[index] + " -> "))
					{
						projectCompleted[index] = true;
						++completedProjects;
						progress_ = static_cast<Float>(completedProjects) / static_cast<Float>(projectNames.size());
						std::lock_guard<std::mutex> lock(statusMutex_);
						statusText_ = std::format("プロジェクト {} / {}", completedProjects, projectNames.size());
					}
				}

				Size compileFlagPosition = line.rfind("/utf-8");
				Size outputPosition = line.find("/OUT:\"");
				if (line.contains("clang-cl.exe") && compileFlagPosition != std::string_view::npos)
				{
					std::string_view sourceFiles = line.substr(compileFlagPosition + 6);
					Size sourceBegin = sourceFiles.find_first_not_of(" \"");
					if (sourceBegin == std::string_view::npos)
					{
						continue;
					}
					Size sourceEnd = sourceFiles.find_last_not_of(" \"");
					std::string_view sourceFile = sourceFiles.substr(sourceBegin, sourceEnd - sourceBegin + 1);

					std::string_view projectName;
					Size intermediatePosition = line.find("\\Intermediate\\");
					if (intermediatePosition != std::string_view::npos)
					{
						Size nameBegin = intermediatePosition + 14;
						Size nameEnd = line.find('\\', nameBegin);
						projectName = line.substr(nameBegin, nameEnd - nameBegin);
					}

					std::lock_guard<std::mutex> lock(statusMutex_);
					detailText_ = projectName.empty() ? std::format("コンパイル中: {}", sourceFile) : std::format("コンパイル中: {} ({})", sourceFile, projectName);
				}
				else if (outputPosition != std::string_view::npos && (line.contains("lld-link.exe") || line.contains("llvm-lib.exe")))
				{
					Size pathBegin = outputPosition + 6;
					Size pathEnd = line.find('"', pathBegin);
					std::string_view outputPath = line.substr(pathBegin, pathEnd - pathBegin);
					Size fileNameBegin = outputPath.find_last_of("\\/");
					std::string_view outputFile = fileNameBegin == std::string_view::npos ? outputPath : outputPath.substr(fileNameBegin + 1);

					std::lock_guard<std::mutex> lock(statusMutex_);
					detailText_ = std::format("リンク中: {}", outputFile);
				}
			}
		};

		if (!RunProcessCaptureOutput(commandLine, outLog, onChunk))
		{
			return false;
		}

		std::time_t now = std::time(nullptr);
		std::tm localTime{};
		localtime_s(&localTime, &now);

		Char timestamp[32];
		std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &localTime);

		std::filesystem::path outputDir = projectRoot / "Package" / (std::string("Build_") + timestamp);

		stage_ = RuntimeBuildStage::Package;
		progress_ = 0.0f;
		{
			std::lock_guard<std::mutex> lock(statusMutex_);
			statusText_.clear();
			detailText_.clear();
		}

		std::string packageLog;
		Bool packageSuccess = Package(projectRoot, outputDir, packageLog);
		outLog += "\n" + packageLog;

		return packageSuccess;
	}

	Bool RuntimeBuilder::Package(const std::filesystem::path& projectRoot, const std::filesystem::path& outputDir, std::string& outLog)
	{
		std::filesystem::path pythonExecutable = projectRoot / "Platform" / "Python" / "python.exe";
		std::filesystem::path packagerScript = projectRoot / "Tools" / "Python" / "RuntimePackager.py";

		std::wstring commandLine =
			L"\"" + pythonExecutable.wstring() + L"\" "
			L"\"" + packagerScript.wstring() + L"\" "
			L"\"" + projectRoot.wstring() + L"\" "
			L"\"" + outputDir.wstring() + L"\"";

		Size parsedLength = 0;
		Uint32 stepIndex = 0;
		Uint32 stepCount = 1;
		std::string stepName;

		auto onChunk = [this, &parsedLength, &stepIndex, &stepCount, &stepName](const std::string& accumulated)
		{
			Size lineEnd = accumulated.find('\n', parsedLength);
			while (lineEnd != std::string::npos)
			{
				std::string_view line(accumulated.data() + parsedLength, lineEnd - parsedLength);
				parsedLength = lineEnd + 1;
				lineEnd = accumulated.find('\n', parsedLength);
				if (!line.empty() && line.back() == '\r')
				{
					line.remove_suffix(1);
				}

				if (line.starts_with("@@progress "))
				{
					std::string_view arguments = line.substr(11);
					const Char* argumentsEnd = arguments.data() + arguments.size();
					Uint32 index = 0;
					Uint32 count = 0;
					std::from_chars_result indexResult = std::from_chars(arguments.data(), argumentsEnd, index);
					if (indexResult.ec != std::errc() || indexResult.ptr == argumentsEnd)
					{
						continue;
					}
					std::from_chars_result countResult = std::from_chars(indexResult.ptr + 1, argumentsEnd, count);
					if (countResult.ec != std::errc() || count == 0 || index == 0)
					{
						continue;
					}

					stepIndex = index;
					stepCount = count;
					stepName = countResult.ptr == argumentsEnd ? std::string() : std::string(countResult.ptr + 1, argumentsEnd);
					progress_ = static_cast<Float>(stepIndex - 1) / static_cast<Float>(stepCount);

					std::lock_guard<std::mutex> lock(statusMutex_);
					statusText_ = stepName;
					detailText_.clear();
				}
				else if (line.starts_with("@@detail ") && stepIndex > 0)
				{
					std::string_view arguments = line.substr(9);
					const Char* argumentsEnd = arguments.data() + arguments.size();
					Uint32 done = 0;
					Uint32 total = 0;
					std::from_chars_result doneResult = std::from_chars(arguments.data(), argumentsEnd, done);
					if (doneResult.ec != std::errc() || doneResult.ptr == argumentsEnd)
					{
						continue;
					}
					std::from_chars_result totalResult = std::from_chars(doneResult.ptr + 1, argumentsEnd, total);
					if (totalResult.ec != std::errc() || total == 0)
					{
						continue;
					}

					progress_ = (static_cast<Float>(stepIndex - 1) + static_cast<Float>(done) / static_cast<Float>(total)) / static_cast<Float>(stepCount);

					std::lock_guard<std::mutex> lock(statusMutex_);
					statusText_ = std::format("{}  {} / {}", stepName, done, total);
					detailText_ = totalResult.ptr == argumentsEnd ? std::string() : std::string(totalResult.ptr + 1, argumentsEnd);
				}
			}
		};

		Bool success = RunProcessCaptureOutput(commandLine, outLog, onChunk);

		std::string filteredLog;
		filteredLog.reserve(outLog.size());
		Size lineBegin = 0;
		while (lineBegin < outLog.size())
		{
			Size lineEnd = outLog.find('\n', lineBegin);
			Size nextLineBegin = lineEnd == std::string::npos ? outLog.size() : lineEnd + 1;
			if (outLog.compare(lineBegin, 2, "@@") != 0)
			{
				filteredLog.append(outLog, lineBegin, nextLineBegin - lineBegin);
			}
			lineBegin = nextLineBegin;
		}
		outLog = std::move(filteredLog);

		return success;
	}
}