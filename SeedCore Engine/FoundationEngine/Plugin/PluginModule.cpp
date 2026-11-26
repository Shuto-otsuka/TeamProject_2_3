#include <FoundationEngine/Plugin/PluginModule.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <Windows.h>
#include <psapi.h>

namespace SeedCore
{
	PluginModule::PluginModule(const std::filesystem::path& sourcePath) : sourcePath_(sourcePath)
	{
		/// No Code
	}

	PluginModule::~PluginModule()
	{
		/// No Code
	}

	Bool PluginModule::Load(World& world, ImGuiContext* imguiContext)
	{
		Uint64 writeTime = GetLastWriteTime(sourcePath_);
		if (writeTime == 0)
		{
			SC_LOG_WARNING("プラグイン: {} が見つかりません。", sourcePath_.filename().string());
			return false;
		}

		CleanupStaleArtifacts();

		std::filesystem::path shadowPath = sourcePath_;
		shadowPath.replace_filename(std::format(L"{}_{}.dll", sourcePath_.stem().wstring(), writeTime));

		std::error_code errorCode;
		constexpr Int maxAttempts = 5;
		for (Int attempt = 0; attempt < maxAttempts; ++attempt)
		{
			errorCode.clear();
			std::filesystem::copy_file(sourcePath_, shadowPath, std::filesystem::copy_options::overwrite_existing, errorCode);
			if (!errorCode)
			{
				break;
			}
			Sleep(50);
		}

		if (errorCode)
		{
			SC_LOG_WARNING("プラグイン: {} のシャドウコピーに失敗しました。", sourcePath_.filename().string());
			return false;
		}

		DynamicArray<String> reflectionKeysBefore = CollectRegistryKeys(ReflectionRegistry::GetRegistry());
		DynamicArray<String> payloadKeysBefore = CollectRegistryKeys(PayloadRegistry::GetRegistry());

		HMODULE handle = LoadLibraryExW(shadowPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (!handle)
		{
			SC_LOG_WARNING("プラグイン: {} のロードに失敗しました。", sourcePath_.filename().string());
			return false;
		}

		registeredReflectionNames_ = FindAddedRegistryKeys(ReflectionRegistry::GetRegistry(), reflectionKeysBefore);
		registeredPayloadNames_ = FindAddedRegistryKeys(PayloadRegistry::GetRegistry(), payloadKeysBefore);

		auto onGameLoad = reinterpret_cast<OnGameLoadFn>(GetProcAddress(handle, "SC_OnGameLoad"));
		auto onGameUnload = reinterpret_cast<OnGameUnloadFn>(GetProcAddress(handle, "SC_OnGameUnload"));
		auto setImGuiContext = reinterpret_cast<SetImGuiContextFn>(GetProcAddress(handle, "SC_SetImGuiContext"));

		if (!onGameLoad || !onGameUnload)
		{
			SC_LOG_WARNING("プラグイン: {} に SC_OnGameLoad/SC_OnGameUnload が見つかりません。", sourcePath_.filename().string());

			UnregisterModuleReflection();

			FreeLibrary(handle);

			std::error_code shadowRemoveError;
			std::filesystem::remove(shadowPath, shadowRemoveError);
			return false;
		}

		handle_ = handle;
		shadowPath_ = shadowPath;
		lastWriteTime_ = writeTime;
		onGameLoad_ = onGameLoad;
		onGameUnload_ = onGameUnload;
		setImGuiContext_ = setImGuiContext;

		if (setImGuiContext_ && imguiContext)
		{
			setImGuiContext_(imguiContext);
		}

		onGameLoad_(world);

		RestoreCapturedComponents(world);

		UnknownComponent::Resolve(world);

		SC_LOG_NOTICE("プラグイン: {} をロードしました。", sourcePath_.filename().string());

		return true;
	}

	void PluginModule::Unload(World& world)
	{
		if (!handle_)
		{
			return;
		}

		DestroyCapturedComponents(world);

		onGameUnload_(world);

		UnregisterModuleReflection();

		FreeLibrary(handle_);

		std::error_code errorCode;
		std::filesystem::remove(shadowPath_, errorCode);

		handle_ = nullptr;
		onGameLoad_ = nullptr;
		onGameUnload_ = nullptr;
		setImGuiContext_ = nullptr;
		shadowPath_.clear();
	}

	Bool PluginModule::Reload(World& world, ImGuiContext* imguiContext)
	{
		Unload(world);
		if (Load(world, imguiContext))
		{
			return true;
		}

		Keep();
		return false;
	}

	void PluginModule::Keep()
	{
		for (const std::pair<Actor, BlueprintComponent>& captured : capturedComponents_)
		{
			UnknownComponent::Keep(captured.first, captured.second);
		}
		capturedComponents_.clear();
	}

	Bool PluginModule::SourceChanged()const
	{
		Uint64 writeTime = GetLastWriteTime(sourcePath_);
		return writeTime != 0 && writeTime != lastWriteTime_;
	}

	const std::filesystem::path& PluginModule::SourcePath()const
	{
		return sourcePath_;
	}

	Bool PluginModule::Loaded()const
	{
		return handle_ != nullptr;
	}

	void PluginModule::CleanupStaleArtifacts()const
	{
		std::error_code errorCode;
		std::filesystem::path directory = sourcePath_.parent_path();
		std::wstring prefix = sourcePath_.stem().wstring() + L"_";

		std::filesystem::directory_iterator iterator(directory, errorCode);
		if (errorCode)
		{
			return;
		}

		for (const auto& entry : iterator)
		{
			std::filesystem::path path = entry.path();
			std::wstring name = path.filename().wstring();

			if (!name.starts_with(prefix))
			{
				continue;
			}

			std::filesystem::path extension = path.extension();
			if (extension != L".dll" && extension != L".pdb")
			{
				continue;
			}

			if (!shadowPath_.empty() && path == shadowPath_)
			{
				continue;
			}

			std::error_code removeError;
			std::filesystem::remove(path, removeError);
		}
	}

	Uint64 PluginModule::GetLastWriteTime(const std::filesystem::path& path)
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

	void PluginModule::UnregisterModuleReflection()
	{
		auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
		for (const String& name : registeredReflectionNames_)
		{
			reflectionRegistry.erase(name);
		}
		registeredReflectionNames_.clear();

		auto& payloadRegistry = PayloadRegistry::GetRegistry();
		for (const String& name : registeredPayloadNames_)
		{
			payloadRegistry.erase(name);
		}
		registeredPayloadNames_.clear();
	}

	void PluginModule::DestroyCapturedComponents(World& world)
	{
		auto isOwnedByLoadedModule = [this](ComponentID id) -> Bool
		{
			if (!handle_)
			{
				return false;
			}

			const void* address = ComponentRegistry::Get(id).construct_;
			if (!address)
			{
				return false;
			}

			MODULEINFO moduleInfo{};
			if (!K32GetModuleInformation(GetCurrentProcess(), handle_, &moduleInfo, sizeof(moduleInfo)))
			{
				return false;
			}

			const Byte* base = static_cast<const Byte*>(moduleInfo.lpBaseOfDll);
			const Byte* target = static_cast<const Byte*>(address);
			return target >= base && target < base + moduleInfo.SizeOfImage;
		};

		capturedComponents_.clear();

		ActorRegistry& actors = WorldAccessor::Actors(world);
		EntityRegistry& entities = WorldAccessor::Entities(world);
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			Actor actor = WorldAccessor::Make(world, current);

			DynamicArray<ComponentID> ownedIDs;
			for (const SparseSetLocation& location : entities.Record(current)->sparseSet_)
			{
				if (location.id_->hooks_ && location.id_->hooks_->upcast_ && isOwnedByLoadedModule(location.id_))
				{
					ownedIDs.push_back(location.id_);
				}
			}

			for (ComponentID id : ownedIDs)
			{
				void* data = actor.GetComponent(id);
				if (data)
				{
					String name = ComponentRegistry::Name(id);
					capturedComponents_.emplace_back(actor, BlueprintComponent::Capture(name, data));
				}

				actor.RemoveComponent(id);
			}
		}

		DynamicArray<ComponentID> ownedComponentIDs;
		for (ComponentID id : ComponentRegistry::Registry())
		{
			if (isOwnedByLoadedModule(id))
			{
				ownedComponentIDs.push_back(id);
			}
		}

		for (ComponentID id : ownedComponentIDs)
		{
			ComponentRegistry::Unregister(id);
		}
	}

	void PluginModule::RestoreCapturedComponents(World& world)
	{
		for (auto& [actor, component] : capturedComponents_)
		{
			ComponentID id = ComponentRegistry::GetComponentID(component.componentName_);
			if (!id)
			{
				UnknownComponent::Keep(actor, component);
				continue;
			}

			actor.AddComponent(id);

			void* data = actor.GetComponent(id);
			if (data)
			{
				component.Apply(data);
			}
		}

		capturedComponents_.clear();
	}
}
