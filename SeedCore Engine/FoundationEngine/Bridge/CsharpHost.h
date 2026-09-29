#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Bridge/NativeApi.h>
#include <FoundationEngine/Bridge/ManagedApi.h>
#include <FoundationEngine/Bridge/CsharpBehaviour.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/Blueprint.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API CsharpHost :public NonTransferable
	{
	public:
		CsharpHost() = default;
		~CsharpHost() = default;

		Bool Initialize(const std::filesystem::path& executableDirectory);

		void Load(World& world);

		void Reload(World& world);

		void Unload(World& world);

	private:
		DynamicArray<std::pair<Actor, BlueprintComponent>> Detach(World& world);

		static void Log(Uint8 level, const Uint8* text, Int32 length);

		static void RegisterScript(void* context, const Uint8* name, Int32 nameLength, const Uint8* category, Int32 categoryLength, Int32 typeIndex, Uint32 hookMask, Int32 blockSize);

		static void RegisterField(void* context, Int32 typeIndex, const Uint8* name, Int32 nameLength, Uint8 type, Int32 offset, Double min, Double max, Uint8 flag, const Uint8* enumName, Int32 enumNameLength, Uint8 assetType);

		static void RegisterEnum(void* context, const Uint8* enumName, Int32 enumNameLength, Int32 value, const Uint8* entryName, Int32 entryNameLength);

		static void Intern(const Uint8* text, Int32 length, Uint8* destination);

		static void* Field(void* context, EntityID entity, const Uint8* component, Int32 componentLength, const Uint8* field, Int32 fieldLength, Uint8 type);

	private:
		using CoreclrInitializeFunction = Int32(*)(const Char* exePath, const Char* appDomainFriendlyName, Int32 propertyCount, const Char** propertyKeys, const Char** propertyValues, void** hostHandle, Uint32* domainID);

		using CoreclrCreateDelegateFunction = Int32(*)(void* hostHandle, Uint32 domainID, const Char* assemblyName, const Char* typeName, const Char* methodName, void** delegate);

		using CoreclrShutdownFunction = Int32(*)(void* hostHandle, Uint32 domainID);

		using InitializeFunction = Int32(*)(NativeApi* nativeApi, ManagedApi* managedApi);

		HMODULE coreclr_ = nullptr;

		void* hostHandle_ = nullptr;

		Uint32 domainID_ = 0;

		CoreclrShutdownFunction shutdown_ = nullptr;

		std::filesystem::path assemblyDirectory_;

		World* world_ = nullptr;

		DynamicArray<String> enumNames_;

		NativeApi nativeApi_{};

		ManagedApi managedApi_{};

		DynamicArray<ResourcePtr<CsharpScriptType>> scriptTypes_;
	};
}