#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/JobSystem/JobScheduler.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
	ResourceCache::ResourceCache()
	{
		/// No Code
	}

	ResourceCache::~ResourceCache()
	{
		Finalize();
	}

	void ResourceCache::Finalize()
	{
		loadExecutor_ = nullptr;

		if (watchHandle_ != INVALID_HANDLE_VALUE)
		{
			FindCloseChangeNotification(watchHandle_);
			watchHandle_ = INVALID_HANDLE_VALUE;
		}

		Unload();

		resourceMap_.clear();
	}

	void ResourceCache::Register(AssetType type, Asset& resource)
	{
		resourceMap_.insert({ type, &resource });
	}

	Asset* ResourceCache::Resource(AssetType type)const
	{
		if (!resourceMap_.contains(type))
		{
			return nullptr;
		}

		return resourceMap_.at(type);
	}

	PrefabPool& ResourceCache::Prefabs()
	{
		return prefabPool_;
	}

	ScenePool& ResourceCache::Scenes()
	{
		return scenePool_;
	}

	void ResourceCache::Reload()
	{
		Rescan();

		AssetContext context{ *this, LoadThread::Main };

		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (asset.isLoaded_ || asset.type_ == AssetType::Unknown)
			{
				continue;
			}

			Asset* resource = Resource(asset.type_);
			if (resource)
			{
				resource->Load(context, asset.assetID_);
			}

			asset.isLoaded_ = true;
		}

		while (watchHandle_ != INVALID_HANDLE_VALUE && WaitForSingleObject(watchHandle_, 0) == WAIT_OBJECT_0)
		{
			FindNextChangeNotification(watchHandle_);
		}

		++revision_;
	}

	void ResourceCache::Reload(Uint32 assetID)
	{
		auto it = assetsMap_.find(assetID);
		if (it == assetsMap_.end() || it->second.type_ == AssetType::Unknown)
		{
			return;
		}

		Asset* resource = Resource(it->second.type_);
		if (!resource)
		{
			return;
		}

		AssetContext context{ *this, LoadThread::Main };
		if (it->second.isLoaded_)
		{
			resource->Unload(assetID);
			it->second.isLoaded_ = false;
		}

		resource->Load(context, assetID);
		it->second.isLoaded_ = true;
	}

	void ResourceCache::Forget(Uint32 assetID)
	{
		auto it = assetsMap_.find(assetID);
		if (it == assetsMap_.end())
		{
			return;
		}

		if (it->second.isLoaded_ && it->second.type_ != AssetType::Unknown)
		{
			Asset* resource = Resource(it->second.type_);
			if (resource)
			{
				resource->Unload(assetID);
			}
		}

		assetsMap_.erase(it);
	}

	void ResourceCache::Watch()
	{
		if (watchHandle_ == INVALID_HANDLE_VALUE)
		{
			std::filesystem::path watchPath = FileLocator::ProjectRoot();
			if (!std::filesystem::exists(watchPath))
			{
				watchPath = FileLocator::EngineRoot();
			}
			watchHandle_ = FindFirstChangeNotificationW(watchPath.wstring().c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
			return;
		}

		if (WaitForSingleObject(watchHandle_, 0) == WAIT_OBJECT_0)
		{
			Reload();
		}
	}

	Uint64 ResourceCache::Revision()const
	{
		return revision_;
	}

	void ResourceCache::Queue()
	{
		scanComplete_.store(false, std::memory_order_release);
		pendingAssetIDs_.clear();
		pendingIndex_ = 0;
		totalAssetCount_.store(0, std::memory_order_release);
		loadedAssetCount_.store(0, std::memory_order_release);
		loadStarted_.store(false, std::memory_order_release);

		Scan(String(FileLocator::EngineRoot().wstring()));

		static const AssetType order[] =
		{
			AssetType::Texture,
			AssetType::Model,
			AssetType::Animation,
			AssetType::MeshCollision,
			AssetType::Material,
			AssetType::Skeleton,
			AssetType::Font,
			AssetType::Effekseer,
			AssetType::Effect,
			AssetType::Audio,
			AssetType::Movie,
			AssetType::Prefab,
			AssetType::Scene,
			AssetType::Skymap,
		};

		for (AssetType type : order)
		{
			for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
			{
				if (asset.type_ == type && !asset.isLoaded_)
				{
					pendingAssetIDs_.push_back(asset.assetID_);
				}
			}
		}

		totalAssetCount_.store(static_cast<Uint32>(pendingAssetIDs_.size()), std::memory_order_release);
		scanComplete_.store(true, std::memory_order_release);
	}

	Bool ResourceCache::Step(LoadThread thread)
	{
		if (!scanComplete_.load(std::memory_order_acquire))
		{
			return false;
		}

		if (pendingIndex_ >= pendingAssetIDs_.size())
		{
			return false;
		}

		Uint32 assetID = pendingAssetIDs_[pendingIndex_++];

		AssetRecord* asset = Record(assetID);
		if (asset && !asset->isLoaded_)
		{
			Asset* resource = Resource(asset->type_);
			if (resource)
			{
				AssetContext context{ *this, thread };
				Benchmark::Begin("Load");
				resource->Load(context, asset->assetID_);
				Benchmark::End("Load");
			}

			asset->isLoaded_ = true;
		}

		loadedAssetCount_.fetch_add(1, std::memory_order_acq_rel);
		return true;
	}

	void ResourceCache::Run()
	{
		Bool expected = false;
		if (!loadStarted_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		{
			return;
		}

		if (!loadExecutor_)
		{
			ResourceRef<JobWorkerInterface> scheduler = benchmark_ ? MakeWorkerInterface<JobScheduler>(*benchmark_) : nullptr;
			loadExecutor_ = MakePtr<JobExecutor>(1, scheduler);
		}

		loadTaskflow_.Clear();
		loadTaskflow_.emplace([this]()
		{
			while (Step(LoadThread::Async))
			{
				/// No Code
			}
		});

		loadExecutor_->Run(loadTaskflow_);
	}

	void ResourceCache::Benchmark(BenchmarkSystem& benchmark)
	{
		benchmark_ = &benchmark;
	}

	Bool ResourceCache::Complete()const
	{
		return scanComplete_.load(std::memory_order_acquire) && pendingIndex_ >= pendingAssetIDs_.size();
	}

	Float ResourceCache::Progress()const
	{
		if (!scanComplete_.load(std::memory_order_acquire))
		{
			return 0.0f;
		}

		Uint32 total = totalAssetCount_.load(std::memory_order_acquire);
		if (total == 0)
		{
			return 1.0f;
		}

		return static_cast<Float>(loadedAssetCount_.load(std::memory_order_acquire)) / static_cast<Float>(total);
	}

	AssetType ResourceCache::Classify(const std::string& extension)
	{
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".dds" || extension == ".texture")
		{
			return AssetType::Texture;
		}

		if (extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".crister")
		{
			return AssetType::Model;
		}

		if (extension == ".animation")
		{
			return AssetType::Animation;
		}
		if (extension == ".collision")
		{
			return AssetType::MeshCollision;
		}
		if (extension == ".material")
		{
			return AssetType::Material;
		}
		if (extension == ".skeleton")
		{
			return AssetType::Skeleton;
		}

		if (extension == ".zephyr")
		{
			return AssetType::Effect;
		}
		if (extension == ".efk" || extension == ".efkefc" || extension == ".effekseer")
		{
			return AssetType::Effekseer;
		}

		if (extension == ".mp3" || extension == ".wav" || extension == ".acb" || extension == ".awb" || extension == ".audio")
		{
			return AssetType::Audio;
		}
		if (extension == ".ttf" || extension == ".otf" || extension == ".ttc")
		{
			return AssetType::Font;
		}
		if (extension == ".mp4" || extension == ".movie")
		{
			return AssetType::Movie;
		}
		if (extension == ".hdr" || extension == ".skymap")
		{
			return AssetType::Skymap;
		}

		if (extension == ".prefab")
		{
			return AssetType::Prefab;
		}
		if (extension == ".scene")
		{
			return AssetType::Scene;
		}
		return AssetType::Unknown;
	}

	AssetRecord* ResourceCache::Record(Uint32 assetID)
	{
		auto it = assetsMap_.find(assetID);
		if (it != assetsMap_.end())
		{
			return &(it->second);
		}
		return nullptr;
	}

	Uint32 ResourceCache::Find(String filename)
	{
		if (searchMap_.contains(filename))
		{
			return searchMap_.at(filename);
		}

		std::string targetFilename = std::filesystem::path(filename.c_str()).filename().string();

		Uint32 matchedAssetID = 0;
		Uint32 matchCount = 0;
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			std::string candidateFilename = std::filesystem::path(asset.path_.c_str()).filename().string();
			if (candidateFilename != targetFilename)
			{
				continue;
			}

			if (matchCount == 0)
			{
				matchedAssetID = asset.assetID_;
			}
			++matchCount;
		}

		if (matchCount > 1)
		{
			SC_LOG_WARNING("ファイル名 \"{}\" のアセットが複数見つかりました。最初に見つかったものを使います。", filename.c_str());
		}

		return matchedAssetID;
	}

	DynamicArray<AssetRecord*> ResourceCache::Search(String key)
	{
		DynamicArray<AssetRecord*> result;

		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (asset.path_.str().contains(key.c_str()))
			{
				result.push_back(&asset);
			}
		}

		return result;
	}

	const FlatMap<Uint32, AssetRecord>& ResourceCache::Records()const
	{
		return assetsMap_;
	}

	void ResourceCache::Scan(String targetPath)
	{
		std::filesystem::path root(targetPath.c_str());
		if (!std::filesystem::exists(root))
		{
			return;
		}

		std::unordered_set<std::string> existingPaths;
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			existingPaths.insert(asset.path_.str());
		}

		std::unordered_map<std::string, std::filesystem::path> orphanedMetas;
		std::unordered_set<std::string> ambiguousOrphanNames;

		for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (it->is_directory())
			{
				std::string directoryName = it->path().filename().string();
				if (excludeDirectories_.contains(directoryName))
				{
					it.disable_recursion_pending();
				}
				continue;
			}

			const std::filesystem::directory_entry& entry = *it;
			if (!entry.is_regular_file() || entry.path().extension().string() != ".meta")
			{
				continue;
			}

			std::filesystem::path formerPath = entry.path();
			formerPath.replace_extension();

			if (std::filesystem::exists(formerPath))
			{
				continue;
			}

			std::string filename = formerPath.filename().string();
			if (ambiguousOrphanNames.contains(filename))
			{
				continue;
			}

			if (orphanedMetas.contains(filename))
			{
				orphanedMetas.erase(filename);
				ambiguousOrphanNames.insert(filename);
			}
			else
			{
				orphanedMetas.insert({ filename, formerPath });
			}
		}

		for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (it->is_directory())
			{
				std::string directoryName = it->path().filename().string();
				if (excludeDirectories_.contains(directoryName))
				{
					it.disable_recursion_pending();
				}
				continue;
			}

			const std::filesystem::directory_entry& entry = *it;

			if (!entry.is_regular_file())
			{
				continue;
			}

			std::string extension = entry.path().extension().string();

			if (!includeExtensions_.contains(extension))
			{
				continue;
			}

			std::string rootPath = std::filesystem::relative(entry.path(), root).string();
			std::ranges::replace(rootPath, '\\', '/');

			if (existingPaths.contains(rootPath))
			{
				continue;
			}

			AssetRecord asset;

			std::string fullPath = std::filesystem::absolute(entry.path()).string();
			std::ranges::replace(fullPath, '\\', '/');
			asset.fullpath_ = String(fullPath);

			asset.path_ = String(rootPath);

			asset.type_ = Classify(extension);

			AssetMeta meta;
			Bool skipMeta = noMetaExtensions_.contains(extension);

			auto mint = [this, &asset, &extension]()
				{
					std::string seed = asset.path_.str();
					if (!generatedExtensions_.contains(extension))
					{
						seed += std::to_string(std::time(nullptr));
					}
					Uint32 guid = static_cast<Uint32>(std::hash<std::string>{}(seed));
					while (assetsMap_.contains(guid))
					{
						guid++;
					}
					return guid;
				};

			if (skipMeta)
			{
				meta.guid_ = static_cast<Uint32>(std::hash<std::string>{}(asset.path_.str()));
				while (assetsMap_.contains(meta.guid_))
				{
					meta.guid_++;
				}
			}
			else
			{
				std::filesystem::path metaPath = entry.path();
				metaPath += ".meta";
				if (std::filesystem::exists(metaPath))
				{
					if (!meta.Read(entry.path()))
					{
						meta = AssetMeta{};
						meta.guid_ = mint();
						meta.Write(entry.path());
					}
				}
				else
				{
					auto orphanIt = orphanedMetas.find(entry.path().filename().string());
					Bool recovered = false;

					if (orphanIt != orphanedMetas.end())
					{
						if (meta.Read(orphanIt->second))
						{
							std::filesystem::path orphanMetaPath = orphanIt->second;
							orphanMetaPath += ".meta";
							std::error_code errorCode;
							std::filesystem::rename(orphanMetaPath, metaPath, errorCode);
							if (errorCode)
							{
								std::filesystem::copy_file(orphanMetaPath, metaPath, std::filesystem::copy_options::overwrite_existing, errorCode);
								std::filesystem::remove(orphanMetaPath, errorCode);
							}

							recovered = true;
						}

						orphanedMetas.erase(orphanIt);
					}

					if (!recovered)
					{
						meta.guid_ = mint();
						meta.Write(entry.path());
					}
				}
			}

			asset.assetID_ = meta.guid_;

			assetsMap_.insert({ asset.assetID_, asset });
			searchMap_.insert(asset.path_, asset.assetID_);
		}
	}

	void ResourceCache::Rescan()
	{
		for (auto it = assetsMap_.begin(); it != assetsMap_.end();)
		{
			if (!std::filesystem::exists(it->second.fullpath_.c_str()))
			{
				if (it->second.isLoaded_)
				{
					Asset* resource = Resource(it->second.type_);
					if (resource)
					{
						resource->Unload(it->second.assetID_);
					}
				}
				it = assetsMap_.erase(it);
			}
			else
			{
				++it;
			}
		}

		searchMap_.clear();
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			searchMap_.insert(asset.path_, asset.assetID_);
		}

		Scan(String(FileLocator::EngineRoot().wstring()));
	}

	void ResourceCache::Unload()
	{
		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (!asset.isLoaded_ || asset.type_ == AssetType::Unknown)
			{
				continue;
			}

			Asset* resource = Resource(asset.type_);
			if (resource)
			{
				resource->Unload(asset.assetID_);
			}

			asset.isLoaded_ = false;
		}

		for (Asset* resource : resourceMap_ | std::ranges::views::values)
		{
			resource->Clear();
		}

		assetsMap_.clear();
		searchMap_.clear();
	}
}
