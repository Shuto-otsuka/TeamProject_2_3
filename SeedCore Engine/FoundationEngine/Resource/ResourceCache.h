#pragma once
#include <FoundationEngine/Utility/ArtMap.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/Scene/ScenePool.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	class FontManager;
	class JobExecutor;
	class BenchmarkSystem;

	class SEEDCORE_API ResourceCache
	{
	public:
		ResourceCache();

		~ResourceCache();

		void Finalize();

		void Register(AssetType type, Asset& resource);

		NODISCARD Asset* Resource(AssetType type)const;

		NODISCARD PrefabPool& Prefabs();

		NODISCARD ScenePool& Scenes();

		void Reload();

		void Reload(Uint32 assetID);

		void Forget(Uint32 assetID);

		void Watch();

		NODISCARD Uint64 Revision()const;

		void Queue();

		Bool Step(LoadThread thread);

		void Run();

		void Benchmark(BenchmarkSystem& benchmark);

		NODISCARD Bool Complete()const;

		NODISCARD Float Progress()const;

		NODISCARD static AssetType Classify(const std::string& extension);

		NODISCARD AssetRecord* Record(Uint32 assetID);

		NODISCARD Uint32 Find(String filename);

		NODISCARD DynamicArray<AssetRecord*> Search(String key);

		NODISCARD const FlatMap<Uint32, AssetRecord>& Records()const;

	private:
		void Scan(String targetPath);

		void Rescan();

		void Unload();

	private:
		FlatMap<Uint32, AssetRecord> assetsMap_;

		ArtMap<String, Uint32> searchMap_;

		FlatMap<AssetType, Asset*> resourceMap_;

		PrefabPool prefabPool_;

		ScenePool scenePool_;

		DynamicArray<Uint32> pendingAssetIDs_;

		std::atomic<Size> pendingIndex_{ 0 };

		std::atomic<Uint32> totalAssetCount_{ 0 };

		std::atomic<Uint32> loadedAssetCount_{ 0 };

		std::atomic<Bool> scanComplete_{ false };

		JobTaskflow loadTaskflow_;

		ResourcePtr<JobExecutor> loadExecutor_;

		BenchmarkSystem* benchmark_ = nullptr;

		std::atomic<Bool> loadStarted_{ false };

		HANDLE watchHandle_ = INVALID_HANDLE_VALUE;

		Uint64 revision_ = 0;

		std::set<std::string_view> includeExtensions_ =
		{
			".png", ".jpg", ".jpeg", ".dds", ".texture",
			".gltf", ".glb", ".fbx", ".crister",
			".animation",
			".collision",
			".material",
			".skeleton",
			".navmesh",
			".zephyr",
			".efk", ".efkefc", ".effekseer", ".efkmodel", ".efkmat", ".efkcurve",
			".mp3", ".wav", ".acb", ".awb", ".audio",
			".ttf", ".otf", ".ttc",
			".mp4", ".movie",
			".hdr", ".skymap",
			".prefab",
			".scene",
			".scg",
			".h", ".cpp", ".cs", ".hlsli", ".hlsl",
		};

		std::set<std::string_view> excludeDirectories_ =
		{
			"AIEngine",
			"AudioEngine",
			"CompiledShaderObject",
			"Editor",
			"External",
			"FoundationEngine",
			"GraphicsEngine",
			"Launcher",
			"Logs",
			"Package",
			"PhysicsEngine",
			"Platform",
			"Runtime",
			"SeedCore",
			"Tools",
			".vs",
			"x64",
			".git",
			".asset",
		};

		std::set<std::string_view> noMetaExtensions_ =
		{
			".h", 
			".cpp", 
			".cs", 
			".hlsli", 
			".hlsl",

			".efkmodel",
			".efkmat",
			".efkcurve",

			".scg",
		};

		std::set<std::string_view> generatedExtensions_ =
		{
			".texture",
			".crister",
			".animation",
			".collision",
			".material",
			".skeleton",
			".navmesh",
			".audio",
			".movie",
			".skymap",
		};
	};
}
