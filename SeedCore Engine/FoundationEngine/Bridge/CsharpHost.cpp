#include <FoundationEngine/Bridge/CsharpHost.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>
#include <FoundationEngine/Log/LogSystem.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Bool CsharpHost::Initialize(const std::filesystem::path& executableDirectory)
	{
		std::filesystem::path buildDirectory = executableDirectory.parent_path();
		std::filesystem::path runtimeDirectory = buildDirectory / "DotNET";
		assemblyDirectory_ = buildDirectory / "Csharp";

		coreclr_ = LoadLibraryW((runtimeDirectory / "coreclr.dll").c_str());
		if (!coreclr_)
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。.NET ランタイム（coreclr.dll）を読み込めません: {}（ビルド後イベントで Platform/DotNET/Runtime がコピーされているか確認してください）", (runtimeDirectory / "coreclr.dll").string());
			return false;
		}

		CoreclrInitializeFunction initialize = reinterpret_cast<CoreclrInitializeFunction>(GetProcAddress(coreclr_, "coreclr_initialize"));
		CoreclrCreateDelegateFunction createDelegate = reinterpret_cast<CoreclrCreateDelegateFunction>(GetProcAddress(coreclr_, "coreclr_create_delegate"));
		shutdown_ = reinterpret_cast<CoreclrShutdownFunction>(GetProcAddress(coreclr_, "coreclr_shutdown"));
		if (initialize == nullptr || createDelegate == nullptr || shutdown_ == nullptr)
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。coreclr.dll から起動用の関数を取得できません: {}（ランタイムのファイルが壊れている可能性があります）", (runtimeDirectory / "coreclr.dll").string());
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

		if (!std::filesystem::exists(assemblyDirectory_ / "SeedCore.Csharp.dll"))
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。SeedCore.Csharp.dll が見つかりません: {}（C# のプロジェクトがビルドされているか確認してください）", (assemblyDirectory_ / "SeedCore.Csharp.dll").string());
			return false;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(assemblyDirectory_))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".dll" || entry.path().filename() == "UserProject.Csharp.dll")
			{
				continue;
			}

			std::u8string path = entry.path().u8string();
			trustedPlatformAssemblies.append(reinterpret_cast<const Char*>(path.c_str()), path.size());
			trustedPlatformAssemblies.push_back(';');
		}

		std::string appPath = reinterpret_cast<const Char*>(assemblyDirectory_.u8string().c_str());
		Wchar executablePath[MAX_PATH]{};
		GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		std::string exePath = reinterpret_cast<const Char*>(std::filesystem::path(executablePath).u8string().c_str());

		const Char* propertyKeys[] = { "TRUSTED_PLATFORM_ASSEMBLIES", "APP_PATHS" };
		const Char* propertyValues[] = { trustedPlatformAssemblies.c_str(), appPath.c_str() };

		Int32 result = initialize(exePath.c_str(), "SeedCore", 2, propertyKeys, propertyValues, &hostHandle_, &domainID_);
		if (result < 0)
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。.NET ランタイムの初期化に失敗しました（HRESULT {:#010x}）", static_cast<Uint32>(result));
			return false;
		}

		auto shutdown = [this]()
			{
				shutdown_(hostHandle_, domainID_);
				hostHandle_ = nullptr;
			};

		void* pointer = nullptr;
		result = createDelegate(hostHandle_, domainID_, "SeedCore.Csharp", "SeedCore.DLLMain", "Initialize", &pointer);
		if (result < 0)
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。SeedCore.DLLMain.Initialize が見つかりません（HRESULT {:#010x}）", static_cast<Uint32>(result));
			shutdown();
			return false;
		}

		nativeApi_.context_ = this;
		nativeApi_.log_ = &CsharpHost::Log;
		nativeApi_.registerScript_ = &CsharpHost::RegisterScript;
		nativeApi_.registerField_ = &CsharpHost::RegisterField;
		nativeApi_.registerEnum_ = &CsharpHost::RegisterEnum;
		nativeApi_.intern_ = &CsharpHost::Intern;
		nativeApi_.field_ = &CsharpHost::Field;
		InitializeFunction entry = reinterpret_cast<InitializeFunction>(pointer);
		result = entry(&nativeApi_, &managedApi_);
		if (result != 0)
		{
			SC_LOG_ERROR("C# ランタイム を起動できませんでした。SeedCore.DLLMain.Initialize が失敗しました（戻り値 {}）", result);
			shutdown();
			return false;
		}

		return true;
	}

	void CsharpHost::Load(World& world)
	{
		if (hostHandle_ == nullptr)
		{
			return;
		}

		world_ = &world;

		std::filesystem::path userAssembly = assemblyDirectory_ / "UserProject.Csharp.dll";
		if (!std::filesystem::exists(userAssembly))
		{
			SC_LOG_WARNING("C# スクリプトを読み込めませんでした。UserProject.Csharp.dll が見つかりません: {}（UserProject.Csharp がビルドされているか確認してください）", userAssembly.string());
			return;
		}

		std::u8string userAssemblyPath = userAssembly.u8string();
		managedApi_.load_(reinterpret_cast<const Uint8*>(userAssemblyPath.c_str()), static_cast<Int32>(userAssemblyPath.size()));
	}

	void CsharpHost::Reload(World& world)
	{
		if (hostHandle_ == nullptr)
		{
			return;
		}

		DynamicArray<std::pair<Actor, BlueprintComponent>> captured = Detach(world);
		managedApi_.unload_();
		Load(world);

		for (const auto& [actor, component] : captured)
		{
			ComponentID id = ComponentRegistry::GetComponentID(component.componentName_);
			if (!id)
			{
				UnknownComponent::Keep(actor, component);
				continue;
			}

			Actor restored = actor;
			restored.AddComponent(id);

			void* data = world.GetComponent(restored.GetEntity(), id);
			if (data)
			{
				ApplyComponent(component, data);
			}
		}

		UnknownComponent::Resolve(world);

		SC_LOG_NOTICE("C# スクリプトをリロードしました。");
	}

	void CsharpHost::Unload(World& world)
	{
		Detach(world);
		world_ = nullptr;

		if (hostHandle_ == nullptr)
		{
			return;
		}

		shutdown_(hostHandle_, domainID_);
		hostHandle_ = nullptr;
	}

	DynamicArray<std::pair<Actor, BlueprintComponent>> CsharpHost::Detach(World& world)
	{
		DynamicArray<std::pair<Actor, BlueprintComponent>> captured;
		for (Actor actor : world.GetActors())
		{
			for (const ResourcePtr<CsharpScriptType>& scriptType : scriptTypes_)
			{
				void* data = world.GetComponent(actor.GetEntity(), scriptType->id_);
				if (!data)
				{
					continue;
				}

				captured.emplace_back(actor, CaptureComponent(ComponentRegistry::Name(scriptType->id_), data));
				actor.RemoveComponent(scriptType->id_);
			}
		}

		for (const ResourcePtr<CsharpScriptType>& scriptType : scriptTypes_)
		{
			ReflectionRegistry::GetRegistry().erase(ComponentRegistry::Name(scriptType->id_));
			world.UnregisterSparseSetStorage(scriptType->id_);
			ComponentRegistry::Unregister(scriptType->id_);
		}

		for (const String& enumName : enumNames_)
		{
			EnumRegistry::GetRegistry().erase(enumName);
		}
		enumNames_.clear();

		scriptTypes_.clear();
		return captured;
	}

	void CsharpHost::Log(Uint8 level, const Uint8* text, Int32 length)
	{
		std::string message(reinterpret_cast<const Char*>(text), static_cast<Size>(length));
		LogSystem::Push(static_cast<LogLevel>(level), message, __FILE__, __LINE__);
	}

	void CsharpHost::RegisterScript(void* context, const Uint8* name, Int32 nameLength, const Uint8* category, Int32 categoryLength, Int32 typeIndex, Uint32 hookMask, Int32 blockSize)
	{
		CsharpHost* host = static_cast<CsharpHost*>(context);
		std::string scriptName(reinterpret_cast<const Char*>(name), static_cast<Size>(nameLength));
		std::string scriptCategory(reinterpret_cast<const Char*>(category), static_cast<Size>(categoryLength));

		if (blockSize < 0 || static_cast<Size>(blockSize) > CsharpBehaviour::blockCapacity_)
		{
			SC_LOG_ERROR("C# スクリプトを登録できませんでした: {}（フィールドの合計 {} バイトが上限 {} バイトを超えています）", scriptName, blockSize, CsharpBehaviour::blockCapacity_);
			return;
		}

		ResourcePtr<CsharpScriptType> scriptType = MakePtr<CsharpScriptType>();
		scriptType->typeIndex_ = typeIndex;
		scriptType->hookMask_ = hookMask;
		scriptType->blockSize_ = blockSize;
		scriptType->managedApi_ = &host->managedApi_;

		ComponentMetadata metadata = Component<CsharpBehaviour>::Metadata(ComponentStorage::SparseSet);
		metadata.createSparseStorage_ = []()->ResourcePtr<InterfaceSparseSetStorage>
			{
				return MakePtr<SparseSetStorage<CsharpBehaviour>>();
			};
		metadata.isComponentBehaviour_ = true;
		metadata.setupLifecycle_ = &CsharpBehaviour::Setup;
		metadata.context_ = scriptType.get();

		ComponentRegistry::Register(String::intern(scriptName), String::intern(scriptCategory), metadata);
		ReflectionRegistry::Register(String::intern(scriptName), &CsharpBehaviour::Reflect);
		scriptType->id_ = ComponentRegistry::GetComponentID(String::intern(scriptName));
		host->scriptTypes_.push_back(std::move(scriptType));

		SC_LOG_NOTICE("C# スクリプトを登録しました: {}（カテゴリ {}、typeIndex {}、フック {:#06x}、フィールド {} バイト）", scriptName, scriptCategory, typeIndex, hookMask, blockSize);
	}

	void CsharpHost::RegisterField(void* context, Int32 typeIndex, const Uint8* name, Int32 nameLength, Uint8 type, Int32 offset, Double min, Double max, Uint8 flag, const Uint8* enumName, Int32 enumNameLength, Uint8 assetType)
	{
		CsharpHost* host = static_cast<CsharpHost*>(context);
		auto it = std::ranges::find_if(host->scriptTypes_, [typeIndex](const ResourcePtr<CsharpScriptType>& scriptType) { return scriptType->typeIndex_ == typeIndex; });
		if (it == host->scriptTypes_.end())
		{
			return;
		}

		CsharpField field;
		field.name_ = String::intern(std::string_view(reinterpret_cast<const Char*>(name), static_cast<Size>(nameLength)));
		field.type_ = static_cast<AttributeType>(type);
		field.offset_ = offset;
		field.min_ = min;
		field.max_ = max;
		field.flag_ = static_cast<ScriptFieldFlag>(flag);
		field.enumName_ = String::intern(std::string_view(reinterpret_cast<const Char*>(enumName), static_cast<Size>(enumNameLength)));
		field.assetType_ = static_cast<PayloadType>(assetType);
		(*it)->fields_.push_back(std::move(field));
	}

	void CsharpHost::RegisterEnum(void* context, const Uint8* enumName, Int32 enumNameLength, Int32 value, const Uint8* entryName, Int32 entryNameLength)
	{
		CsharpHost* host = static_cast<CsharpHost*>(context);
		String typeName = String::intern(std::string_view(reinterpret_cast<const Char*>(enumName), static_cast<Size>(enumNameLength)));
		String name = String::intern(std::string_view(reinterpret_cast<const Char*>(entryName), static_cast<Size>(entryNameLength)));

		if (std::ranges::find(host->enumNames_, typeName) == host->enumNames_.end())
		{
			host->enumNames_.push_back(typeName);
			EnumRegistry::GetRegistry()[typeName].clear();
		}
		EnumRegistry::GetRegistry()[typeName].push_back({ value, name });
	}

	void CsharpHost::Intern(const Uint8* text, Int32 length, Uint8* destination)
	{
		*reinterpret_cast<String*>(destination) = String::intern(std::string_view(reinterpret_cast<const Char*>(text), static_cast<Size>(length)));
	}

	void* CsharpHost::Field(void* context, EntityID entity, const Uint8* component, Int32 componentLength, const Uint8* field, Int32 fieldLength, Uint8 type)
	{
		CsharpHost* host = static_cast<CsharpHost*>(context);
		if (host->world_ == nullptr)
		{
			return nullptr;
		}

		Actor actor = host->world_->GetActor(entity);
		String componentName = String::intern(std::string_view(reinterpret_cast<const Char*>(component), static_cast<Size>(componentLength)));
		String fieldName = String::intern(std::string_view(reinterpret_cast<const Char*>(field), static_cast<Size>(fieldLength)));

		switch (static_cast<AttributeType>(type))
		{
		case AttributeType::Int:
			return &actor.Field<Int>(componentName, fieldName);
		case AttributeType::Float:
			return &actor.Field<Float>(componentName, fieldName);
		case AttributeType::Bool:
			return &actor.Field<Bool>(componentName, fieldName);
		case AttributeType::Vector2:
			return &actor.Field<Vector2>(componentName, fieldName);
		case AttributeType::Vector3:
			return &actor.Field<Vector3>(componentName, fieldName);
		case AttributeType::Color:
			return &actor.Field<Color>(componentName, fieldName);
		default:
			return nullptr;
		}
	}
}