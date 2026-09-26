#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Bridge/CsharpNativeApi.h>

namespace SeedCore
{
	class SEEDCORE_API CsharpHost :public NonTransferable
	{
	public:
		CsharpHost() = default;
		~CsharpHost() = default;

		Bool Initialize(const std::filesystem::path& executableDirectory);

		void Finalize();

	private:
		static void Log(Uint8 level, const Uint8* text, Int32 length);

	private:
		using CoreclrInitializeFunction = Int32(*)(const Char* exePath, const Char* appDomainFriendlyName, Int32 propertyCount, const Char** propertyKeys, const Char** propertyValues, void** hostHandle, Uint32* domainID);

		using CoreclrCreateDelegateFunction = Int32(*)(void* hostHandle, Uint32 domainID, const Char* assemblyName, const Char* typeName, const Char* methodName, void** delegate);

		using CoreclrShutdownFunction = Int32(*)(void* hostHandle, Uint32 domainID);

		using InitializeFunction = Int32(*)(CsharpNativeApi* nativeApi);

		HMODULE coreclr_ = nullptr;

		void* hostHandle_ = nullptr;

		Uint32 domainID_ = 0;

		CoreclrShutdownFunction shutdown_ = nullptr;

		CsharpNativeApi nativeApi_{};
	};
}