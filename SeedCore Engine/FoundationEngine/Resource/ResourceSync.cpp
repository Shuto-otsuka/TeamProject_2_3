#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/ResourceSync.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>

namespace SeedCore
{
	ResourceSync::ResourceSync(const std::filesystem::path& projectRoot) : projectRoot_(projectRoot)
	{
		std::ifstream stream(projectRoot_ / ".asset" / "config.json");
		if (!stream)
		{
			return;
		}

		nlohmann::json config = nlohmann::json::parse(stream, nullptr, false);
		if (!config.is_object())
		{
			return;
		}

		config_.clientId_ = String(config.value("clientId", ""));
		config_.clientSecret_ = String(config.value("clientSecret", ""));
		config_.catalogDocumentId_ = String(config.value("catalogDocumentId", ""));
		config_.blobFolderId_ = String(config.value("blobFolderId", ""));

		config_.assetsFolderId_ = String(config.value("assetsFolderId", ""));

		config_.workspace_ = String(config.value("workspace", "UserProject"));

		configured_ = !config_.catalogDocumentId_.str().empty();
		if (configured_)
		{
			worker_ = MakePtr<SharingWorker>(projectRoot_, config_);
			worker_->Start();
		}
	}

	ResourceSync::~ResourceSync()
	{
		if (worker_)
		{
			worker_->Stop();
		}
	}

	void ResourceSync::Update()
	{
		if (worker_)
		{
			snapshot_ = worker_->Snapshot();
		}
	}

	void ResourceSync::ConsumeChangedAsset(DynamicArray<Uint32>& assets)
	{
		if (worker_)
		{
			worker_->ConsumeChanged(assets);
		}
	}

	void ResourceSync::ConsumeImportedAsset(DynamicArray<String>& paths)
	{
		if (worker_)
		{
			worker_->ConsumeImported(paths);
		}
	}

	void ResourceSync::FetchReferenced(World& world)
	{
		if (!worker_ || !snapshot_.online_ || !fetchReferencedRequested_)
		{
			return;
		}
		fetchReferencedRequested_ = false;

		DynamicArray<Uint32> referenced;
		ActorRegistry& actors = WorldAccessor::Actors(world);
		const PrefabRegistry* prefabs = WorldAccessor::Services(world).Get<PrefabRegistry>();
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			Actor actor = WorldAccessor::Make(world, current);

			const PrefabRecord* prefab = prefabs ? prefabs->Record(current) : nullptr;
			if (prefab && prefab->assetID_ != 0)
			{
				referenced.push_back(prefab->assetID_);
			}

			for (const auto& [typeName, payload] : PayloadRegistry::GetRegistry())
			{
				ComponentID id = ComponentRegistry::GetComponentID(typeName);
				if (!id)
				{
					continue;
				}
				void* data = actor.GetComponent(id);
				if (!data)
				{
					continue;
				}

				DynamicArray<FieldInfo> fields;
				payload(data, fields);
				for (const FieldInfo& field : fields)
				{
					if (field.assetType_ == PayloadType::None || field.array_.add_)
					{
						continue;
					}
					void* value = field.directPtr_ ? field.directPtr_ : static_cast<Uint8*>(data) + field.offset_;
					referenced.push_back(*static_cast<Uint32*>(value));
				}
			}
		}

		std::set<Uint32> requested;
		for (Uint32 assetId : referenced)
		{
			if (assetId == 0 || !RemoteOnly(assetId) || !requested.insert(assetId).second)
			{
				continue;
			}
			RequestGet(assetId);
		}
	}

	Bool ResourceSync::Configured()const
	{
		return configured_;
	}

	Bool ResourceSync::Online()const
	{
		return snapshot_.online_;
	}

	Uint64 ResourceSync::Revision()const
	{
		return snapshot_.revision_;
	}

	void ResourceSync::Gather(DynamicArray<AssetRecord>& assets)const
	{
		for (const SharedAsset& asset : snapshot_.assets_)
		{
			std::filesystem::path local = projectRoot_ / config_.workspace_.str() / asset.path_.w_str();
			if (std::filesystem::exists(local))
			{
				continue;
			}

			AssetRecord record;
			record.assetID_ = asset.runtimeId_;
			record.type_ = ResourceCache::Classify(std::filesystem::path(asset.path_.str()).extension().string());
			record.fullpath_ = String(local.generic_wstring());
			record.path_ = String(std::filesystem::path(config_.workspace_.str()).generic_string() + "/" + asset.path_.str());
			assets.push_back(record);
		}
	}

	Bool ResourceSync::RemoteOnly(Uint32 assetId)const
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!asset)
		{
			return false;
		}

		return !std::filesystem::exists(projectRoot_ / config_.workspace_.str() / asset->path_.w_str());
	}

	Bool ResourceSync::Modified(Uint32 assetId)const
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!asset)
		{
			return false;
		}

		for (const SharedProgress& progress : snapshot_.progress_)
		{
			if (progress.assetId_ == asset->id_)
			{
				return progress.modified_;
			}
		}
		return false;
	}

	Bool ResourceSync::Conflicted(Uint32 assetId)const
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!asset)
		{
			return false;
		}

		for (const SharedProgress& progress : snapshot_.progress_)
		{
			if (progress.assetId_ == asset->id_)
			{
				return progress.conflicted_;
			}
		}
		return false;
	}

	Bool ResourceSync::Managed(const std::filesystem::path& path)const
	{
		if (!configured_)
		{
			return false;
		}

		if (!snapshot_.online_)
		{
			return true;
		}

		String logical = Logical(path);
		if (logical.str().empty())
		{
			return false;
		}

		for (const SharedAsset& asset : snapshot_.assets_)
		{
			for (const SharedFile& file : asset.files_)
			{
				if (file.path_ == logical || file.path_.str().rfind(logical.str() + "/", 0) == 0)
				{
					return true;
				}
			}
		}
		return false;
	}

	Bool ResourceSync::Shareable(const std::filesystem::path& path)const
	{
		if (!configured_ || Logical(path).str().empty())
		{
			return false;
		}
		return !sourceExtensions_.contains(path.extension().string());
	}

	void ResourceSync::RequestGet(Uint32 assetId)
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!worker_ || !asset)
		{
			return;
		}

		SharingRequest request;
		request.action_ = SharingAction::Get;
		request.assetId_ = asset->id_;
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestAdopt(Uint32 assetId)
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!worker_ || !asset)
		{
			return;
		}

		SharingRequest request;
		request.action_ = SharingAction::Adopt;
		request.assetId_ = asset->id_;
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestPublish(Uint32 assetId)
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!worker_ || !asset)
		{
			return;
		}

		SharingRequest request;
		request.action_ = SharingAction::Publish;
		request.assetId_ = asset->id_;
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestRegister(const AssetRecord& asset)
	{
		std::filesystem::path local(asset.fullpath_.str());
		String logical = Logical(local);
		if (!worker_ || logical.str().empty() || !Shareable(local))
		{
			return;
		}

		SharingRequest request;
		request.action_ = SharingAction::Register;
		request.path_ = logical;
		request.runtimeId_ = asset.assetID_;
		request.type_ = static_cast<Int32>(asset.type_);
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestUnshare(Uint32 assetId)
	{
		const SharedAsset* asset = GetAsset(assetId);
		if (!worker_ || !asset)
		{
			return;
		}

		SharingRequest request;
		request.action_ = SharingAction::Unshare;
		request.assetId_ = asset->id_;
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestRefresh()
	{
		if (!worker_)
		{
			return;
		}
		fetchReferencedRequested_ = true;

		SharingRequest request;
		request.action_ = SharingAction::Refresh;
		worker_->Enqueue(request);
	}

	void ResourceSync::RequestOpenScene(const std::filesystem::path& path)
	{
		if (!worker_)
		{
			return;
		}

		const SharedAsset* asset = GetAsset(path);
		SharingRequest request;
		request.action_ = SharingAction::OpenScene;
		request.assetId_ = asset ? asset->id_ : String();
		worker_->Enqueue(request);
	}

	const SharedAsset* ResourceSync::GetAsset(Uint32 assetId)const
	{
		for (const SharedAsset& asset : snapshot_.assets_)
		{
			if (asset.runtimeId_ == assetId)
			{
				return &asset;
			}
		}
		return nullptr;
	}

	const SharedAsset* ResourceSync::GetAsset(const std::filesystem::path& path)const
	{
		String logical = Logical(path);
		if (logical.str().empty())
		{
			return nullptr;
		}

		for (const SharedAsset& asset : snapshot_.assets_)
		{
			if (asset.path_ == logical)
			{
				return &asset;
			}
		}
		return nullptr;
	}

	const String& ResourceSync::Operation()const
	{
		return snapshot_.operation_;
	}

	const String& ResourceSync::Error()const
	{
		return snapshot_.error_;
	}

	String ResourceSync::Logical(const std::filesystem::path& path)const
	{
		std::filesystem::path workspace = (projectRoot_ / config_.workspace_.str()).lexically_normal();
		std::filesystem::path relative = path.lexically_normal().lexically_relative(workspace);

		if (relative.empty() || *relative.begin() == "..")
		{
			return String();
		}
		return String(relative.generic_wstring());
	}
}
