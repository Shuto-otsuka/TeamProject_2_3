#pragma once
#include <FoundationEngine/Plugin/PluginModule.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API PluginHost :public NonTransferable
	{
	public:
		PluginHost() = default;
		~PluginHost() = default;

		Bool Initialize(World& world);

		void Load(World& world);

		void Unload(World& world);

		void Tick(World& world);

		NODISCARD PluginModule* Find(const std::filesystem::path& stem)const;

		void ReloadModule(World& world, PluginModule& module);

	private:
		void ScanAndLoad(World& world);

	private:
		SC_CONSTEX Uint64 stableWindowMilliseconds_ = 500;

		SC_CONSTEX Uint64 rescanIntervalMilliseconds_ = 500;

		struct Entry
		{
			ResourcePtr<PluginModule> module_;

			Uint64 pendingWriteTime_ = 0;

			Uint64 pendingStableSinceTick_ = 0;
		};

		std::filesystem::path pluginDirectory_;

		ImGuiContext* imguiContext_ = nullptr;

		DynamicArray<Entry> entries_;

		DynamicArray<String> rejectedFileNames_;

		Uint64 lastRescanTick_ = 0;
	};
}
