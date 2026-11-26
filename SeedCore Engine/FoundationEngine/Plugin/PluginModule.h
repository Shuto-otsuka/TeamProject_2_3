#pragma once
#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>

namespace SeedCore
{
	class World;
	class Actor;

	class SEEDCORE_API PluginModule :public NonTransferable
	{
	public:
		explicit PluginModule(const std::filesystem::path& sourcePath);

		~PluginModule();

		Bool Load(World& world, ImGuiContext* imguiContext);

		void Unload(World& world);

		Bool Reload(World& world, ImGuiContext* imguiContext);

		void Keep();

		NODISCARD Bool SourceChanged()const;

		NODISCARD const std::filesystem::path& SourcePath()const;

		NODISCARD Bool Loaded()const;

		void CleanupStaleArtifacts()const;

		NODISCARD static Uint64 GetLastWriteTime(const std::filesystem::path& path);

	private:
		template<typename Map>
		static DynamicArray<String> CollectRegistryKeys(const Map& registry)
		{
			DynamicArray<String> keys;
			std::ranges::copy(registry | std::views::keys, std::back_inserter(keys));
			return keys;
		}

		template<typename Map>
		static DynamicArray<String> FindAddedRegistryKeys(const Map& registry, const DynamicArray<String>& previousKeys)
		{
			DynamicArray<String> added;
			std::ranges::copy_if(registry | std::views::keys, std::back_inserter(added), [&previousKeys](const String& name){ return !std::ranges::contains(previousKeys, name); });
			return added;
		}

		void UnregisterModuleReflection();

		void DestroyCapturedComponents(World& world);

		void RestoreCapturedComponents(World& world);

	private:
		using OnGameLoadFn = void(*)(World&);
		using OnGameUnloadFn = void(*)(World&);
		using SetImGuiContextFn = void(*)(ImGuiContext*);

		std::filesystem::path sourcePath_;

		HMODULE handle_ = nullptr;

		std::filesystem::path shadowPath_;

		Uint64 lastWriteTime_ = 0;

		OnGameLoadFn onGameLoad_ = nullptr;
		OnGameUnloadFn onGameUnload_ = nullptr;
		SetImGuiContextFn setImGuiContext_ = nullptr;

		DynamicArray<std::pair<Actor, BlueprintComponent>> capturedComponents_;

		DynamicArray<String> registeredReflectionNames_;

		DynamicArray<String> registeredPayloadNames_;
	};
}
