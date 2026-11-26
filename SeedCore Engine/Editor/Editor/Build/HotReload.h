#pragma once
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class PluginHost;
	class PluginModule;
	class CsharpHost;

	class HotReload :public NonTransferable
	{
	public:
		HotReload() = default;
		~HotReload();

		Bool Initialize(PluginHost& pluginHost, CsharpHost& csharpHost);

		void Tick(World& world);

	private:
		struct BuildTask
		{
			HANDLE process_ = nullptr;

			HANDLE outputPipe_ = nullptr;

			std::string output_;

			Bool reloadRequested_ = false;

			Uint64 lastTriggeredWriteTime_ = 0;

			Uint64 pendingWriteTime_ = 0;

			Uint64 pendingStableSinceTick_ = 0;

			Uint64 lastScanTick_ = 0;
		};

		PluginHost* pluginHost_ = nullptr;

		PluginModule* userProjectPlugin_ = nullptr;

		CsharpHost* csharpHost_ = nullptr;

		std::filesystem::path msbuildPath_;

		Bool msbuildResolved_ = false;

		BuildTask cplusplusBuild_;

		BuildTask csharpBuild_;

		Bool cplusplusReloadDeferred_ = false;

		Bool csharpReloadDeferred_ = false;

		void TriggerCplusplusBuild();

		void TriggerCsharpBuild();

		static Bool Launch(BuildTask& task, std::wstring commandLine, const Char* label);

		static void Drain(BuildTask& task);

		static void Poll(BuildTask& task, const Char* label);

		static Bool Settled(BuildTask& task, Uint64 writeTime, Uint64 nowTick);

		NODISCARD static std::filesystem::path UserProjectSourceDirectory();

		NODISCARD static std::filesystem::path UserProjectVcxprojPath();

		NODISCARD static std::filesystem::path UserProjectCsprojPath();

		NODISCARD static Uint64 GetLastWriteTime(const std::filesystem::path& path);

		NODISCARD static Uint64 ScanSourceLastWriteTime(Bool csharp);

		NODISCARD static std::filesystem::path FindMSBuild();
	};
}
