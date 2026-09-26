#include <FoundationEngine/Bridge/CsharpHost.h>
#include <FoundationEngine/Log/LogSystem.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Bool CsharpHost::Initialize(const std::filesystem::path& executableDirectory)
	{
		std::filesystem::path buildDirectory = executableDirectory.parent_path();
		std::filesystem::path runtimeDirectory = buildDirectory / "DotNET";
		std::filesystem::path assemblyDirectory = buildDirectory / "Csharp";

		coreclr_ = LoadLibraryW((runtimeDirectory / "coreclr.dll").c_str());
		if (!coreclr_)
		{
			SC_LOG_ERROR("C# を起動できませんでした。.NET ランタイム（coreclr.dll）を読み込めません: {}（ビルド後イベントで Platform/DotNET/Runtime がコピーされているか確認してください）", (runtimeDirectory / "coreclr.dll").string());
			return false;
		}

		CoreclrInitializeFunction initialize = reinterpret_cast<CoreclrInitializeFunction>(GetProcAddress(coreclr_, "coreclr_initialize"));
		CoreclrCreateDelegateFunction createDelegate = reinterpret_cast<CoreclrCreateDelegateFunction>(GetProcAddress(coreclr_, "coreclr_create_delegate"));
		shutdown_ = reinterpret_cast<CoreclrShutdownFunction>(GetProcAddress(coreclr_, "coreclr_shutdown"));
		if (initialize == nullptr || createDelegate == nullptr || shutdown_ == nullptr)
		{
			SC_LOG_ERROR("C# を起動できませんでした。coreclr.dll から起動用の関数を取得できません: {}（ランタイムのファイルが壊れている可能性があります）", (runtimeDirectory / "coreclr.dll").string());
			return false;
		}

		std::string trustedPlatformAssemblies;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(runtimeDirectory))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".dll")
			{
				continue;
			}

			std::u8string path = entry.path().u8string();
			trustedPlatformAssemblies.append(reinterpret_cast<const Char*>(path.c_str()), path.size());
			trustedPlatformAssemblies.push_back(';');
		}

		if (!std::filesystem::exists(assemblyDirectory / "SeedCore.Csharp.dll"))
		{
			SC_LOG_ERROR("C# を起動できませんでした。SeedCore.Csharp.dll が見つかりません: {}（C# のプロジェクトがビルドされているか確認してください）", (assemblyDirectory / "SeedCore.Csharp.dll").string());
			return false;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(assemblyDirectory))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".dll" || entry.path().filename() == "UserProject.Csharp.dll")
			{
				continue;
			}

			std::u8string path = entry.path().u8string();
			trustedPlatformAssemblies.append(reinterpret_cast<const Char*>(path.c_str()), path.size());
			trustedPlatformAssemblies.push_back(';');
		}

		std::string appPath = reinterpret_cast<const Char*>(assemblyDirectory.u8string().c_str());
		std::string exePath = reinterpret_cast<const Char*>((executableDirectory / "Editor.exe").u8string().c_str());

		const Char* propertyKeys[] = { "TRUSTED_PLATFORM_ASSEMBLIES", "APP_PATHS" };
		const Char* propertyValues[] = { trustedPlatformAssemblies.c_str(), appPath.c_str() };

		Int32 result = initialize(exePath.c_str(), "SeedCore", 2, propertyKeys, propertyValues, &hostHandle_, &domainID_);
		if (result < 0)
		{
			SC_LOG_ERROR("C# を起動できませんでした。.NET ランタイムの初期化に失敗しました（HRESULT {:#010x}）", static_cast<Uint32>(result));
			return false;
		}

		void* pointer = nullptr;
		result = createDelegate(hostHandle_, domainID_, "SeedCore.Csharp", "SeedCore.DLLMain", "Initialize", &pointer);
		if (result < 0)
		{
			SC_LOG_ERROR("C# を起動できませんでした。SeedCore.DLLMain.Initialize が見つかりません（HRESULT {:#010x}）", static_cast<Uint32>(result));
			Finalize();
			return false;
		}

		nativeApi_.log_ = &CsharpHost::Log;
		InitializeFunction entry = reinterpret_cast<InitializeFunction>(pointer);
		result = entry(&nativeApi_);
		if (result != 0)
		{
			SC_LOG_ERROR("C# を起動できませんでした。SeedCore.DLLMain.Initialize が失敗しました（戻り値 {}）", result);
			Finalize();
			return false;
		}

		return true;
	}

	void CsharpHost::Finalize()
	{
		if (hostHandle_ == nullptr)
		{
			return;
		}

		shutdown_(hostHandle_, domainID_);
		hostHandle_ = nullptr;
	}

	void CsharpHost::Log(Uint8 level, const Uint8* text, Int32 length)
	{
		std::string message(reinterpret_cast<const Char*>(text), static_cast<Size>(length));
		LogSystem::Push(static_cast<LogLevel>(level), message, __FILE__, __LINE__);
	}
}