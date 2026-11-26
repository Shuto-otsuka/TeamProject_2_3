#pragma once

namespace SeedCore
{
	class SEEDCORE_API AftermathCrashTracker
	{
	public:
		static void Enable();

		static void Create(ID3D12Device* device);

		static String Report();

		static void Disable();

	private:
		using ScAftermathAddGpuCrashDumpDescription = void(*)(uint32_t key, const char* value);

		enum ScAftermathGpuCrashDumpDescriptionKey : uint32_t
		{
			ScAftermathGpuCrashDumpDescriptionKeyApplicationName = 0x1
		};

		static const Char* DumpDirectory();

		static void OnCrashDump(const void* gpuCrashDump, Uint32 gpuCrashDumpSize, void* userData);

		static void OnShaderDebugInfo(const void* shaderDebugInfo, Uint32 shaderDebugInfoSize, void* userData);

		static void OnDescription(ScAftermathAddGpuCrashDumpDescription addValue, void* userData);

	private:
		static std::mutex mutex_;

		static String lastCrashDumpPath_;

		static Uint32 shaderDebugInfoCount_;
	};
}
