#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/SharingWorker.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Resource/Sharing/SharedScene.h>

namespace SeedCore
{
	static Bool SplitScene(const nlohmann::json& scene, std::unordered_map<String, nlohmann::json>& parts, String& error);

	SharingWorker::SharingWorker(const std::filesystem::path& projectRoot, const SharingConfig& config) : projectRoot_(projectRoot), config_(config), auth_(http_, projectRoot / ".asset" / "credentials", config.clientId_, config.clientSecret_), document_(http_, auth_), drive_(http_, auth_), catalog_(document_, config.catalogDocumentId_)
	{
		LoadWorkspace();
	}

	SharingWorker::~SharingWorker()
	{
		Stop();
	}

	void SharingWorker::Start()
	{
		if (running_)
		{
			return;
		}

		running_ = true;
		thread_ = std::thread(&SharingWorker::Run, this);
	}

	void SharingWorker::Stop()
	{
		if (!running_)
		{
			return;
		}

		running_ = false;
		if (thread_.joinable())
		{
			thread_.join();
		}
	}

	void SharingWorker::Enqueue(const SharingRequest& request)
	{
		std::lock_guard<std::mutex> guard(requestMutex_);
		requests_.push(request);
	}

	SharingSnapshot SharingWorker::Snapshot()const
	{
		std::lock_guard<std::mutex> guard(snapshotMutex_);
		return snapshot_;
	}

	void SharingWorker::ConsumeChanged(DynamicArray<Uint32>& assets)
	{
		std::lock_guard<std::mutex> guard(snapshotMutex_);
		for (Uint32 assetId : changed_)
		{
			assets.push_back(assetId);
		}
		changed_.clear();
	}

	void SharingWorker::ConsumeImported(DynamicArray<String>& paths)
	{
		std::lock_guard<std::mutex> guard(snapshotMutex_);
		for (const String& path : imported_)
		{
			paths.push_back(path);
		}
		imported_.clear();
	}

	void SharingWorker::Run()
	{
		if (!auth_.SignedIn() && !auth_.SignIn())
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.error_ = auth_.Error();
			return;
		}

		Double nextRefresh = 0.0;
		while (running_)
		{
			Double now = std::chrono::duration<Double>(std::chrono::steady_clock::now().time_since_epoch()).count();

			SharingRequest request;
			Bool hasRequest = false;
			{
				std::lock_guard<std::mutex> guard(requestMutex_);
				if (!requests_.empty())
				{
					request = requests_.front();
					requests_.pop();
					hasRequest = true;
				}
			}
			if (hasRequest)
			{
				Process(request);
				continue;
			}

			if (now >= nextRefresh)
			{
				nextRefresh = now + 5.0;
				Bool read = catalog_.Refresh();
				{
					std::lock_guard<std::mutex> guard(snapshotMutex_);
					snapshot_.online_ = read;
					snapshot_.error_ = read ? String() : catalog_.Error();
				}
				Capture();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		SaveWorkspace();
	}

	void SharingWorker::Process(const SharingRequest& request)
	{
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);

			snapshot_.error_ = String();
			switch (request.action_)
			{
			case SharingAction::Refresh:
				snapshot_.operation_ = String("共有ライブラリを確認中");
				break;

			case SharingAction::Get:
				snapshot_.operation_ = String("取得中");
				break;

			case SharingAction::Adopt:
				snapshot_.operation_ = String("ライブラリの版で置き換え中");
				break;

			case SharingAction::Register:
				snapshot_.operation_ = String("共有ライブラリへ登録中");
				break;

			case SharingAction::Publish:
				snapshot_.operation_ = String("公開中");
				break;

			case SharingAction::Unshare:
				snapshot_.operation_ = String("共有を解除中");
				break;

			default:
				break;
			}
		}

		Bool done = false;
		switch (request.action_)
		{
		case SharingAction::Refresh:
			done = catalog_.Refresh();
			if (done)
			{
				Pull();
			}
			break;

		case SharingAction::Get:
			done = Get(request.assetId_);
			break;

		case SharingAction::Adopt:
			done = Adopt(request.assetId_);
			break;

		case SharingAction::Register:
			done = Register(request);
			break;

		case SharingAction::Publish:
			done = Publish(request.assetId_);
			break;

		case SharingAction::Unshare:
			done = Unshare(request.assetId_);
			break;

		case SharingAction::OpenScene:

			openScene_ = request.assetId_;
			done = true;
			break;
		}

		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.operation_ = String();

			if (!done && snapshot_.error_.str().empty())
			{
				snapshot_.error_ = catalog_.Error();
			}
		}
		Capture();

		if (done && (request.action_ == SharingAction::Publish || request.action_ == SharingAction::Unshare) && !config_.blobFolderId_.str().empty())
		{
			std::set<std::string> inUse;
			Bool readable = true;
			for (const SharedAsset& asset : catalog_.Assets())
			{
				for (const SharedFile& file : asset.files_)
				{
					inUse.insert(file.driveId_.str());
				}

				if (asset.scene_ && !asset.sceneDocumentId_.str().empty())
				{
					SharedScene shared(document_, asset.sceneDocumentId_);
					readable = readable && shared.Refresh();
					for (const ScenePart& part : shared.Parts())
					{
						inUse.insert(part.driveId_.str());
					}
				}
			}

			DynamicArray<DriveEntry> blobs;
			if (readable && drive_.List(config_.blobFolderId_, blobs))
			{
				std::string cutoff = std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now() - std::chrono::hours(1)));
				for (const DriveEntry& blob : blobs)
				{
					if (blob.folder_ || !blob.canTrash_ || inUse.contains(blob.id_.str()) || blob.modified_.str().empty() || blob.modified_.str() >= cutoff)
					{
						continue;
					}

					if (!drive_.Trash(blob.id_))
					{
						break;
					}
				}
			}
		}
	}

	void SharingWorker::Pull()
	{
		std::unordered_map<std::string, DriveEntry> placed;
		Bool complete = false;
		if (!config_.assetsFolderId_.str().empty())
		{
			std::set<std::string> sharedPaths;
			std::set<std::string> sharedIds;
			for (const SharedAsset& asset : catalog_.Assets())
			{
				for (const SharedFile& file : asset.files_)
				{
					sharedPaths.insert(file.path_.str());
					sharedIds.insert(file.driveId_.str());
				}
			}
			complete = Mirror(projectRoot_ / config_.workspace_.str() / "Assets", config_.assetsFolderId_, sharedPaths, sharedIds, placed);
			Follow(placed);
		}
		Prune(placed, complete);

		Size recorded = workspace_.size() + sceneBases_.size();
		std::erase_if(workspace_, [this](const std::pair<const String, WorkspaceRecord>& record) { return !catalog_.Find(record.first); });
		std::erase_if(sceneBases_, [this](const std::pair<const String, SceneBase>& record) { return !catalog_.Find(record.first); });
		if (!openScene_.str().empty() && !catalog_.Find(openScene_))
		{
			openScene_ = String();
		}
		if (recorded != workspace_.size() + sceneBases_.size())
		{
			SaveWorkspace();
		}

		for (const SharedAsset& asset : catalog_.Assets())
		{
			if (asset.scene_)
			{
				if (!std::filesystem::exists(Local(asset.path_)))
				{
					Get(asset.id_);
				}
				continue;
			}

			auto record = workspace_.find(asset.id_);
			Bool current = record != workspace_.end() && record->second.revision_ >= asset.revision_;
			if (current)
			{
				for (const SharedFile& file : asset.files_)
				{
					current = current && std::filesystem::exists(Local(file.path_));
				}
			}

			if (!current)
			{
				Get(asset.id_);
			}
		}

		if (!openScene_.str().empty())
		{
			Get(openScene_);
		}
	}

	void SharingWorker::Follow(const std::unordered_map<std::string, DriveEntry>& placed)
	{
		DynamicArray<SharedAsset> followed;
		for (const SharedAsset& asset : catalog_.Assets())
		{
			SharedAsset updated = asset;
			Bool changed = false;
			for (SharedFile& file : updated.files_)
			{
				std::filesystem::path logical(file.path_.w_str());
				if (logical.extension() == ".meta" || logical.empty() || *logical.begin() != "Assets")
				{
					continue;
				}

				auto found = placed.find(file.path_.str());
				if (found == placed.end() || found->second.hash_.str().empty())
				{
					continue;
				}

				Bool moved = found->second.id_ != file.driveId_;
				Bool replaced = !asset.scene_ && found->second.hash_ != file.hash_;
				if (!moved && !replaced)
				{
					continue;
				}
				file.driveId_ = found->second.id_;
				if (replaced)
				{
					file.hash_ = found->second.hash_;
					file.size_ = found->second.size_;
				}
				changed = true;
			}
			if (changed)
			{
				followed.push_back(updated);
			}
		}

		for (const SharedAsset& asset : followed)
		{
			Uint64 revision = 0;
			catalog_.Publish(asset.id_, asset.files_, asset.dependencies_, asset.revision_, revision);
		}
	}

	void SharingWorker::Prune(const std::unordered_map<std::string, DriveEntry>& placed, Bool complete)
	{
		std::set<std::string> alive;

		std::set<std::string> stored;
		for (const std::pair<const std::string, DriveEntry>& entry : placed)
		{
			alive.insert(entry.second.id_.str());
		}

		if (!config_.blobFolderId_.str().empty())
		{
			DynamicArray<DriveEntry> blobs;
			if (!drive_.List(config_.blobFolderId_, blobs))
			{
				return;
			}
			for (const DriveEntry& blob : blobs)
			{
				alive.insert(blob.id_.str());
				stored.insert(blob.id_.str());
			}
		}

		DynamicArray<String> gone;
		for (const SharedAsset& asset : catalog_.Assets())
		{
			Bool missing = false;

			DynamicArray<String> suspects;
			for (const SharedFile& file : asset.files_)
			{
				std::filesystem::path logical(file.path_.w_str());
				if (complete && stored.contains(file.driveId_.str()) && logical.extension() != ".meta" && !logical.empty() && *logical.begin() == "Assets" && !placed.contains(file.path_.str()))
				{
					missing = true;
				}

				if (!file.driveId_.str().empty() && !alive.contains(file.driveId_.str()))
				{
					suspects.push_back(file.driveId_);
				}
			}

			if (asset.scene_ && !asset.sceneDocumentId_.str().empty())
			{
				suspects.push_back(asset.sceneDocumentId_);
			}

			for (const String& suspect : suspects)
			{
				if (missing)
				{
					break;
				}

				Bool exists = true;
				if (!drive_.Exists(suspect, exists))
				{
					break;
				}
				missing = !exists;
			}

			if (missing)
			{
				gone.push_back(asset.id_);
			}
		}

		for (const String& assetId : gone)
		{
			catalog_.Remove(assetId);
		}
	}

	Bool SharingWorker::Get(const String& assetId)
	{
		const SharedAsset* asset = catalog_.Find(assetId);
		if (!asset)
		{
			return false;
		}
		if (asset->scene_ && !asset->sceneDocumentId_.str().empty())
		{
			SharedScene shared(document_, asset->sceneDocumentId_);
			if (!shared.Refresh())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = shared.Error();
				return false;
			}

			if (shared.Parts().empty())
			{
				return true;
			}

			auto base = sceneBases_.find(assetId);
			Bool based = base != sceneBases_.end();
			Bool moved = !based || shared.Parts().size() != base->second.parts_.size();
			if (based)
			{
				for (const ScenePart& part : shared.Parts())
				{
					auto known = base->second.parts_.find(part.scope_);
					moved = moved || known == base->second.parts_.end() || known->second.revision_ != part.revision_;
				}
			}
			if (!moved)
			{
				return true;
			}

			std::unordered_map<String, nlohmann::json> parts;
			for (const ScenePart& part : shared.Parts())
			{
				if (part.deleted_)
				{
					continue;
				}

				if (!part.data_.str().empty())
				{
					parts[part.scope_] = nlohmann::json::parse(part.data_.str(), nullptr, false);
					continue;
				}
	
				std::filesystem::path staging = projectRoot_ / ".asset" / "incoming" / part.hash_.str();
				if (!drive_.Fetch(part.driveId_, staging))
				{
					std::lock_guard<std::mutex> guard(snapshotMutex_);
					snapshot_.error_ = drive_.Error();
					return false;
				}
				std::ifstream stream(staging, std::ios::binary);
				parts[part.scope_] = nlohmann::json::parse(stream, nullptr, false);
				stream.close();
	
				std::error_code errorCode;
				std::filesystem::remove(staging, errorCode);
			}

			std::filesystem::path current = Local(asset->path_);
			if (based && std::filesystem::exists(current))
			{
				std::ifstream reader(current);
				nlohmann::json local = nlohmann::json::parse(reader, nullptr, false);
				reader.close();

				std::unordered_map<String, nlohmann::json> localParts;
				String unreadable;
				if (SplitScene(local, localParts, unreadable))
				{
					for (const std::pair<const String, nlohmann::json>& part : localParts)
					{
						std::string text = part.second.dump();
						DynamicArray<Byte> digest = Sha256::Hash(reinterpret_cast<const Byte*>(text.data()), text.size());
						std::string hash;
						for (Byte value : digest)
						{
							hash += std::format("{:02x}", static_cast<Uint8>(value));
						}

						auto known = base->second.parts_.find(part.first);
						const ScenePart* remote = shared.Find(part.first);
						Bool edited = known == base->second.parts_.end() || known->second.hash_ != String(hash);
						Bool overtaken = remote && (known == base->second.parts_.end() || remote->revision_ != known->second.revision_);
						if (edited && !overtaken)
						{
							parts[part.first] = part.second;
						}
					}

					for (const std::pair<const String, ScenePartBase>& known : base->second.parts_)
					{
						const ScenePart* remote = shared.Find(known.first);
						if (!known.second.hash_.str().empty() && !localParts.contains(known.first) && (!remote || remote->revision_ == known.second.revision_))
						{
							parts.erase(known.first);
						}
					}
				}
			}

			std::unordered_map<std::string, nlohmann::json> transforms;
			std::unordered_map<std::string, std::map<std::string, nlohmann::json>> components;
			for (const std::pair<const String, nlohmann::json>& part : parts)
			{
				const std::string& scope = part.first.str();
				Size slash = scope.find('/');
				if (scope.rfind("entity:", 0) != 0 || slash == std::string::npos)
				{
					continue;
				}

				std::string identifier = scope.substr(7, slash - 7);
				std::string member = scope.substr(slash + 1);
				if (member == "transform")
				{
					transforms[identifier] = part.second;
				}
				else if (member.rfind("component:", 0) == 0)
				{
					components[identifier][member.substr(10)] = part.second;
				}
			}

			auto context = parts.find(String("context"));
			auto structure = parts.find(String("structure"));
			if (context == parts.end() || structure == parts.end() || !structure->second.is_array())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = String("Scene の断片が揃っていないため組み立てられませんでした。");
				return false;
			}

			nlohmann::json scene = context->second;
			scene["nodes"] = nlohmann::json::array();

			std::unordered_map<std::string, Size> placed;
			for (const nlohmann::json& link : structure->second)
			{
				std::string identifier = link.value("id", "");
				auto entity = parts.find(String(std::format("entity:{}", identifier)));
				if (identifier.empty() || entity == parts.end() || placed.contains(identifier))
				{
					std::lock_guard<std::mutex> guard(snapshotMutex_);
					snapshot_.error_ = String("Scene の階層と断片が食い違っているため組み立てられませんでした。");
					return false;
				}

				nlohmann::json node = entity->second;

				auto transform = transforms.find(identifier);
				if (transform != transforms.end() && transform->second.is_object())
				{
					for (auto& field : transform->second.items())
					{
						node[field.key()] = field.value();
					}
				}

				std::map<std::string, nlohmann::json> pieces;
				auto found = components.find(identifier);
				if (found != components.end())
				{
					pieces = found->second;
				}

				nlohmann::json list = nlohmann::json::array();
				if (node.contains("components") && node["components"].is_array())
				{
					for (const nlohmann::json& inlineComponent : node["components"])
					{
						std::string name = inlineComponent.value("component", "");
						if (!pieces.contains(name))
						{
							list.push_back(inlineComponent);
						}
					}
				}
				if (node.contains("componentOrder") && node["componentOrder"].is_array())
				{
					for (const nlohmann::json& name : node["componentOrder"])
					{
						auto piece = name.is_string() ? pieces.find(name.get<std::string>()) : pieces.end();
						if (piece != pieces.end())
						{
							list.push_back(piece->second);
							pieces.erase(piece);
						}
					}
				}
				for (const std::pair<const std::string, nlohmann::json>& piece : pieces)
				{
					list.push_back(piece.second);
				}
				node["components"] = list;
				node.erase("componentOrder");

				node["parentIndex"] = -1;
				if (!link["parent"].is_null())
				{
					auto parent = placed.find(link["parent"].get<std::string>());
					if (parent == placed.end())
					{
						std::lock_guard<std::mutex> guard(snapshotMutex_);
						snapshot_.error_ = String("Scene の親子関係を復元できませんでした。");
						return false;
					}
					node["parentIndex"] = static_cast<Int32>(parent->second);
				}
	
				placed[identifier] = scene["nodes"].size();
				scene["nodes"].push_back(node);
			}

			std::filesystem::path local = Local(asset->path_);
			std::filesystem::path staging = local;
			staging += ".download";
			std::error_code errorCode;
			std::filesystem::create_directories(local.parent_path(), errorCode);
			std::ofstream writer(staging);
			writer << scene.dump(1, '\t');
			writer.close();
			std::filesystem::rename(staging, local, errorCode);
			if (errorCode)
			{
				std::filesystem::remove(staging, errorCode);
				return false;
			}

			SceneBase written;
			for (const ScenePart& part : shared.Parts())
			{
				written.parts_[part.scope_] = ScenePartBase{ part.revision_, part.deleted_ ? String() : part.hash_ };
			}
			sceneBases_[assetId] = written;
			SaveWorkspace();

			std::lock_guard<std::mutex> guard(snapshotMutex_);
			changed_.push_back(asset->runtimeId_);
			return true;
		}

		for (const String& dependency : asset->dependencies_)
		{
			Get(dependency);
		}

		for (const SharedFile& file : asset->files_)
		{
			std::filesystem::path local = Local(file.path_);

			DynamicArray<Byte> digest = Sha256::Hash(local);
			std::string current;
			for (Byte value : digest)
			{
				current += std::format("{:02x}", static_cast<Uint8>(value));
			}
			if (current == file.hash_.str())
			{
				continue;
			}

			std::filesystem::path staging = local;
			staging += ".download";
			if (!drive_.Fetch(file.driveId_, staging))
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = drive_.Error();
				return false;
			}

			DynamicArray<Byte> arrived = Sha256::Hash(staging);
			std::string received;
			for (Byte value : arrived)
			{
				received += std::format("{:02x}", static_cast<Uint8>(value));
			}
			if (received != file.hash_.str())
			{
				std::error_code errorCode;
				std::filesystem::remove(staging, errorCode);
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = String(std::format("\"{}\" の Drive 上の中身がカタログと一致しないため取得しませんでした。公開の途中であれば、少し待ってからもう一度「最新を取得」してください。", file.path_.str()));
				return false;
			}

			std::error_code errorCode;
			std::filesystem::rename(staging, local, errorCode);
			if (errorCode)
			{
				std::filesystem::remove(staging, errorCode);
				return false;
			}

			std::lock_guard<std::mutex> guard(snapshotMutex_);
			changed_.push_back(asset->runtimeId_);
		}

		WorkspaceRecord record;
		record.revision_ = asset->revision_;
		for (const SharedFile& file : asset->files_)
		{
			std::error_code errorCode;
			record.files_.push_back(WorkspaceFile{ file.path_, file.hash_, file.size_, std::filesystem::last_write_time(Local(file.path_), errorCode).time_since_epoch().count() });
		}
		workspace_[assetId] = record;
		SaveWorkspace();
		return true;
	}

	Bool SharingWorker::Adopt(const String& assetId)
	{
		workspace_.erase(assetId);

		const SharedAsset* asset = catalog_.Find(assetId);
		if (asset && asset->scene_)
		{
			sceneBases_.erase(assetId);

			std::ifstream reader(Local(asset->path_));
			nlohmann::json local = nlohmann::json::parse(reader, nullptr, false);
			reader.close();

			std::unordered_map<String, nlohmann::json> localParts;
			String unreadable;
			if (SplitScene(local, localParts, unreadable))
			{
				SceneBase replaced;
				for (const std::pair<const String, nlohmann::json>& part : localParts)
				{
					std::string text = part.second.dump();
					DynamicArray<Byte> digest = Sha256::Hash(reinterpret_cast<const Byte*>(text.data()), text.size());
					std::string hash;
					for (Byte value : digest)
					{
						hash += std::format("{:02x}", static_cast<Uint8>(value));
					}
					replaced.parts_[part.first] = ScenePartBase{ 0, String(hash) };
				}
				sceneBases_[assetId] = replaced;
			}
		}
		return Get(assetId);
	}

	Bool SharingWorker::Publish(const String& assetId)
	{
		if (!catalog_.Refresh())
		{
			return false;
		}
		const SharedAsset* asset = catalog_.Find(assetId);
		if (!asset)
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.error_ = String("そのアセットはもう共有ライブラリにありません。");
			return false;
		}
		if (asset->scene_ && !asset->sceneDocumentId_.str().empty())
		{
			std::ifstream stream(Local(asset->path_));
			nlohmann::json scene = nlohmann::json::parse(stream, nullptr, false);
			stream.close();

			std::unordered_map<String, nlohmann::json> parts;
			String unsplittable;
			if (!SplitScene(scene, parts, unsplittable))
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = unsplittable;
				return false;
			}

			SharedScene shared(document_, asset->sceneDocumentId_);
			if (!shared.Refresh())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = shared.Error();
				return false;
			}

			auto base = sceneBases_.find(assetId);
			Bool based = base != sceneBases_.end();
			if (!based && !shared.Parts().empty())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = String(std::format("\"{}\" はこの PC でまだ取得していないため公開しませんでした。「最新を取得」してから編集し直してください。", asset->path_.str()));
				return false;
			}

			DynamicArray<ScenePartChange> changes;
			DynamicArray<String> conflicts;

			std::unordered_map<String, ScenePartBase> matched;
			for (const std::pair<const String, nlohmann::json>& part : parts)
			{
				std::string text = part.second.dump();
				DynamicArray<Byte> digest = Sha256::Hash(reinterpret_cast<const Byte*>(text.data()), text.size());
				std::string hash;
				for (Byte value : digest)
				{
					hash += std::format("{:02x}", static_cast<Uint8>(value));
				}

				const ScenePart* remote = shared.Find(part.first);
				if (remote && !remote->deleted_ && remote->hash_ == String(hash))
				{
					matched[part.first] = ScenePartBase{ remote->revision_, remote->hash_ };
					continue;
				}

				const ScenePartBase* known = nullptr;
				if (based)
				{
					auto found = base->second.parts_.find(part.first);
					known = found == base->second.parts_.end() ? nullptr : &found->second;
				}
				if (known && known->hash_ == String(hash))
				{
					continue;
				}

				if (based && remote && remote->revision_ > (known ? known->revision_ : 0))
				{
					conflicts.push_back(part.first);
					continue;
				}

				ScenePartChange change;
				change.scope_ = part.first;
				change.baseRevision_ = remote ? remote->revision_ : 0;
				change.hash_ = String(hash);
				change.size_ = text.size();

				if (text.size() <= 64 * 1024)
				{
					change.data_ = String(text);
				}
				else
				{
					std::filesystem::path staging = projectRoot_ / ".asset" / "outgoing" / hash;
					std::error_code errorCode;
					std::filesystem::create_directories(staging.parent_path(), errorCode);
					std::ofstream writer(staging, std::ios::binary);
					writer << text;
					writer.close();

					change.driveId_ = drive_.Find(change.hash_, config_.blobFolderId_);
					if (change.driveId_.str().empty())
					{
						change.driveId_ = drive_.Upload(staging, change.hash_, config_.blobFolderId_);
					}
					std::filesystem::remove(staging, errorCode);
					if (change.driveId_.str().empty())
					{
						std::lock_guard<std::mutex> guard(snapshotMutex_);
						snapshot_.error_ = drive_.Error();
						return false;
					}
				}
				changes.push_back(change);
			}

			if (based)
			{
				for (const std::pair<const String, ScenePartBase>& known : base->second.parts_)
				{
					if (parts.contains(known.first))
					{
						continue;
					}

					const ScenePart* remote = shared.Find(known.first);
					if (!remote || remote->deleted_)
					{
						continue;
					}

					if (remote->revision_ > known.second.revision_)
					{
						conflicts.push_back(known.first);
						continue;
					}

					ScenePartChange change;
					change.scope_ = remote->scope_;
					change.baseRevision_ = remote->revision_;
					change.deleted_ = true;
					changes.push_back(change);
				}
			}

			if (!conflicts.empty())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = String(std::format("他のメンバーが先に公開した部分と、手元の変更がぶつかっているため公開しませんでした（{} 件、例: {}）。「最新を取得」すると、その部分はライブラリの版になります。", conflicts.size(), conflicts.front().str()));
				return false;
			}

			if (!shared.Publish(changes))
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = shared.Error();
				return false;
			}

			SceneBase& updated = sceneBases_[assetId];
			for (const ScenePartChange& change : changes)
			{
				updated.parts_[change.scope_] = ScenePartBase{ change.baseRevision_ + 1, change.deleted_ ? String() : change.hash_ };
			}
			for (const std::pair<const String, ScenePartBase>& entry : matched)
			{
				updated.parts_[entry.first] = entry.second;
			}
			SaveWorkspace();
			return true;
		}

		auto previous = workspace_.find(assetId);
		if (previous == workspace_.end() || previous->second.revision_ < asset->revision_)
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.error_ = String(std::format("\"{}\" は他のメンバーが先に公開しているため公開しませんでした。「最新を取得」で取り込んでから編集し直してください。", asset->path_.str()));
			return false;
		}
		Uint64 baseRevision = asset->revision_;

		DynamicArray<String> uploaded;
		DynamicArray<String> replaced;

		DynamicArray<SharedFile> files;
		for (const SharedFile& file : asset->files_)
		{
			SharedFile stored;
			if (!Store(file.path_, stored, uploaded, replaced))
			{
				for (const String& fileId : uploaded)
				{
					drive_.Trash(fileId);
				}
				return false;
			}
			files.push_back(stored);
		}

		for (const String& companion : Companions(asset->path_))
		{
			if (std::ranges::any_of(files, [&companion](const SharedFile& file) { return file.path_ == companion; }))
			{
				continue;
			}

			SharedFile stored;
			if (!Store(companion, stored, uploaded, replaced))
			{
				for (const String& fileId : uploaded)
				{
					drive_.Trash(fileId);
				}
				return false;
			}
			files.push_back(stored);
		}

		Uint64 revision = 0;
		if (!catalog_.Publish(assetId, files, asset->dependencies_, baseRevision, revision))
		{
			for (const String& fileId : uploaded)
			{
				drive_.Trash(fileId);
			}
			return false;
		}
		for (const String& fileId : replaced)
		{
			drive_.Trash(fileId);
		}

		WorkspaceRecord record;
		record.revision_ = revision;
		for (const SharedFile& file : files)
		{
			std::error_code errorCode;
			record.files_.push_back(WorkspaceFile{ file.path_, file.hash_, file.size_, std::filesystem::last_write_time(Local(file.path_), errorCode).time_since_epoch().count() });
		}
		workspace_[assetId] = record;
		SaveWorkspace();
		return true;
	}

	static Bool SplitScene(const nlohmann::json& scene, std::unordered_map<String, nlohmann::json>& parts, String& error)
	{
		if (!scene.is_object() || !scene.contains("nodes") || !scene["nodes"].is_array())
		{
			error = String("Scene の形が想定と違うため分割できませんでした。");
			return false;
		}

		nlohmann::json context = nlohmann::json::object();
		for (auto& entry : scene.items())
		{
			if (entry.key() != "nodes")
			{
				context[entry.key()] = entry.value();
			}
		}
		parts[String("context")] = context;

		nlohmann::json structure = nlohmann::json::array();
		for (Size index = 0; index < scene["nodes"].size(); ++index)
		{
			const nlohmann::json& node = scene["nodes"][index];

			std::string identifier = node.value("collaborationId", "");
			if (identifier.empty() || parts.contains(String(std::format("entity:{}", identifier))))
			{
				error = String("共同編集用のIDを持たない、あるいは重複している Actor があります。");
				return false;
			}

			Int32 parent = node.value("parentIndex", Int32(-1));
			nlohmann::json link;
			link["id"] = identifier;
			link["parent"] = parent >= 0 && parent < static_cast<Int32>(index) ? nlohmann::json(scene["nodes"][static_cast<Size>(parent)].value("collaborationId", "")) : nlohmann::json(nullptr);
			structure.push_back(link);

			nlohmann::json entity = node;
			entity.erase("parentIndex");

			nlohmann::json transform = nlohmann::json::object();
			static const std::string transformKeys[] = { "position", "pivot", "rotation", "scale" };
			for (const std::string& key : transformKeys)
			{
				if (entity.contains(key))
				{
					transform[key] = entity[key];
					entity.erase(key);
				}
			}
			parts[String(std::format("entity:{}/transform", identifier))] = transform;

			nlohmann::json order = nlohmann::json::array();
			if (entity.contains("components") && entity["components"].is_array())
			{
				for (const nlohmann::json& component : entity["components"])
				{
					std::string name = component.value("component", "");
					String scope = String(std::format("entity:{}/component:{}", identifier, name));
					if (name.empty() || parts.contains(scope))
					{
						error = String(std::format("名前の無い、あるいは重複している Component を持つ Actor があります: {}", entity.value("name", "")));
						return false;
					}
					parts[scope] = component;
					order.push_back(name);
				}
			}
			entity.erase("components");
			entity["componentOrder"] = order;
			parts[String(std::format("entity:{}", identifier))] = entity;
		}
		parts[String("structure")] = structure;
		return true;
	}

	Bool SharingWorker::Register(const SharingRequest& request)
	{
		const SharedAsset* existing = catalog_.FindPath(request.path_);
		if (existing)
		{
			return Adopt(existing->id_);
		}

		SharedAsset asset;

		std::random_device randomDevice;
		asset.id_ = String(std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:08x}{:04x}", randomDevice(), randomDevice() >> 16, randomDevice() >> 16, randomDevice() >> 16, randomDevice(), randomDevice() >> 16));
		asset.path_ = request.path_;
		asset.runtimeId_ = request.runtimeId_;
		asset.type_ = request.type_;

		DynamicArray<String> uploaded;
		DynamicArray<String> replaced;
		SharedFile main;
		SharedFile meta;
		if (!Store(request.path_, main, uploaded, replaced) || !Store(String(std::format("{}.meta", request.path_.str())), meta, uploaded, replaced))
		{
			for (const String& fileId : uploaded)
			{
				drive_.Trash(fileId);
			}
			return false;
		}
		asset.files_.push_back(main);
		asset.files_.push_back(meta);

		for (const String& companion : Companions(request.path_))
		{
			SharedFile stored;
			if (!Store(companion, stored, uploaded, replaced))
			{
				for (const String& fileId : uploaded)
				{
					drive_.Trash(fileId);
				}
				return false;
			}
			asset.files_.push_back(stored);
		}

		if (!catalog_.Register(asset))
		{
			for (const String& fileId : uploaded)
			{
				drive_.Trash(fileId);
			}

			if (catalog_.Refresh())
			{
				const SharedAsset* winner = catalog_.FindPath(request.path_);
				if (winner)
				{
					return Adopt(winner->id_);
				}
			}
			return false;
		}

		for (const String& fileId : replaced)
		{
			drive_.Trash(fileId);
		}

		WorkspaceRecord record;
		record.revision_ = 1;
		for (const SharedFile& file : asset.files_)
		{
			std::error_code errorCode;
			record.files_.push_back(WorkspaceFile{ file.path_, file.hash_, file.size_, std::filesystem::last_write_time(Local(file.path_), errorCode).time_since_epoch().count() });
		}
		workspace_[asset.id_] = record;
		SaveWorkspace();
		return true;
	}

	Bool SharingWorker::Unshare(const String& assetId)
	{
		const SharedAsset* found = catalog_.Find(assetId);
		if (!found)
		{
			return false;
		}

		SharedAsset asset = *found;
		if (!catalog_.Remove(assetId))
		{
			return false;
		}

		if (asset.scene_ && !asset.sceneDocumentId_.str().empty())
		{
			drive_.Trash(asset.sceneDocumentId_);
			if (openScene_ == assetId)
			{
				openScene_ = String();
			}
			sceneBases_.erase(assetId);
		}

		for (const SharedFile& file : asset.files_)
		{
			std::filesystem::path logical(file.path_.w_str());
			if (config_.assetsFolderId_.str().empty() || logical.empty() || *logical.begin() != "Assets")
			{
				continue;
			}

			String driveId = config_.assetsFolderId_;
			for (const std::filesystem::path& segment : logical.lexically_relative("Assets"))
			{
				driveId = drive_.Find(String(segment.generic_wstring()), driveId);
				if (driveId.str().empty())
				{
					break;
				}
			}

			if (!driveId.str().empty())
			{
				drive_.Trash(driveId);
			}
		}

		workspace_.erase(assetId);
		SaveWorkspace();
		return true;
	}

	Bool SharingWorker::Mirror(const std::filesystem::path& localFolder, const String& driveFolderId, const std::set<std::string>& sharedPaths, const std::set<std::string>& sharedIds, std::unordered_map<std::string, DriveEntry>& placed)
	{
		DynamicArray<DriveEntry> entries;
		if (!drive_.List(driveFolderId, entries))
		{
			return false;
		}

		Bool complete = true;
		for (const DriveEntry& entry : entries)
		{
			std::filesystem::path local = localFolder / entry.name_.w_str();
			if (entry.folder_)
			{
				std::error_code errorCode;
				std::filesystem::create_directories(local, errorCode);
				complete = !errorCode && Mirror(local, entry.id_, sharedPaths, sharedIds, placed) && complete;
				continue;
			}

			String logical = String(local.lexically_relative(projectRoot_ / config_.workspace_.str()).generic_wstring());

			auto known = placed.find(logical.str());
			if (known == placed.end() || !sharedIds.contains(known->second.id_.str()))
			{
				placed[logical.str()] = entry;
			}

			if (local.extension() == ".meta" || sharedPaths.contains(logical.str()) || sharedIds.contains(entry.id_.str()))
			{
				continue;
			}

			DynamicArray<Byte> digest = Sha256::Hash(local);
			std::string current;
			for (Byte value : digest)
			{
				current += std::format("{:02x}", static_cast<Uint8>(value));
			}

			std::error_code errorCode;
			Bool held = entry.hash_.str().empty() ? std::filesystem::file_size(local, errorCode) == entry.size_ && !errorCode : current == entry.hash_.str();

			std::filesystem::path meta = local;
			meta += ".meta";
			if (held && !std::filesystem::exists(meta))
			{
				continue;
			}
			if (!held)
			{
				std::filesystem::path staging = local;
				staging += ".download";
				if (!drive_.Fetch(entry.id_, staging))
				{
					continue;
				}

				Bool metaWritten = false;
				if (!std::filesystem::exists(meta))
				{
					std::string rootPath = std::filesystem::relative(local, projectRoot_, errorCode).string();
					std::ranges::replace(rootPath, '\\', '/');
					if (!errorCode && !rootPath.empty())
					{
						AssetMeta assetMeta;
						assetMeta.guid_ = static_cast<Uint32>(std::hash<std::string>{}(rootPath));
						metaWritten = assetMeta.Write(local);
					}
				}

				std::filesystem::rename(staging, local, errorCode);
				if (errorCode)
				{
					std::filesystem::remove(staging, errorCode);

					if (metaWritten)
					{
						std::filesystem::remove(meta, errorCode);
					}
					continue;
				}
			}

			std::lock_guard<std::mutex> guard(snapshotMutex_);
			imported_.push_back(logical);
		}
		return complete;
	}

	Bool SharingWorker::Store(const String& logicalPath, SharedFile& stored, DynamicArray<String>& uploaded, DynamicArray<String>& replaced)
	{
		std::filesystem::path local = Local(logicalPath);

		DynamicArray<Byte> digest = Sha256::Hash(local);
		if (digest.empty())
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.error_ = String(std::format("\"{}\" を読み取れませんでした。", logicalPath.str()));
			return false;
		}

		std::string hash;
		for (Byte value : digest)
		{
			hash += std::format("{:02x}", static_cast<Uint8>(value));
		}

		std::error_code errorCode;
		stored.path_ = logicalPath;
		stored.hash_ = String(hash);
		stored.size_ = std::filesystem::file_size(local, errorCode);

		std::filesystem::path logical(logicalPath.w_str());
		if (!config_.assetsFolderId_.str().empty() && logical.extension() != ".meta" && !logical.empty() && *logical.begin() == "Assets")
		{
			String folderId = config_.assetsFolderId_;
			for (const std::filesystem::path& segment : logical.parent_path().lexically_relative("Assets"))
			{
				if (segment == ".")
				{
					continue;
				}

				String name = String(segment.generic_wstring());
				String childId = drive_.Find(name, folderId);
				if (childId.str().empty())
				{
					childId = drive_.Folder(name, folderId);
				}
				if (childId.str().empty())
				{
					std::lock_guard<std::mutex> guard(snapshotMutex_);
					snapshot_.error_ = drive_.Error();
					return false;
				}
				folderId = childId;
			}

			DynamicArray<DriveEntry> entries;
			if (!drive_.List(folderId, entries))
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = drive_.Error();
				return false;
			}

			std::wstring fileName = logical.filename().wstring();
			auto found = std::ranges::find_if(entries, [&fileName](const DriveEntry& entry) { return !entry.folder_ && entry.name_.w_str() == fileName; });
			if (found != entries.end() && found->hash_ == stored.hash_)
			{
				stored.driveId_ = found->id_;
				return true;
			}

			stored.driveId_ = drive_.Upload(local, String(fileName), folderId);
			if (stored.driveId_.str().empty())
			{
				std::lock_guard<std::mutex> guard(snapshotMutex_);
				snapshot_.error_ = drive_.Error();
				return false;
			}
			uploaded.push_back(stored.driveId_);
			if (found != entries.end())
			{
				replaced.push_back(found->id_);
			}
			return true;
		}

		stored.driveId_ = drive_.Find(stored.hash_, config_.blobFolderId_);
		if (!stored.driveId_.str().empty())
		{
			return true;
		}

		stored.driveId_ = drive_.Upload(local, stored.hash_, config_.blobFolderId_);
		if (stored.driveId_.str().empty())
		{
			std::lock_guard<std::mutex> guard(snapshotMutex_);
			snapshot_.error_ = drive_.Error();
			return false;
		}
		return true;
	}

	DynamicArray<String> SharingWorker::Companions(const String& logicalPath)const
	{
		DynamicArray<String> companions;
		std::filesystem::path logical(logicalPath.str());
		if (logical.extension() != ".gltf")
		{
			return companions;
		}

		std::ifstream stream(Local(logicalPath));
		nlohmann::json gltf = nlohmann::json::parse(stream, nullptr, false);
		if (!gltf.is_object())
		{
			return companions;
		}

		static const std::string sections[] = { "buffers", "images" };
		for (const std::string& section : sections)
		{
			if (!gltf.contains(section) || !gltf[section].is_array())
			{
				continue;
			}

			for (const nlohmann::json& entry : gltf[section])
			{
				std::string uri = entry.value("uri", "");
				if (uri.empty() || uri.rfind("data:", 0) == 0)
				{
					continue;
				}

				std::string decoded;
				for (Size index = 0; index < uri.size(); ++index)
				{
					if (uri[index] == '%' && index + 2 < uri.size() && std::isxdigit(static_cast<Uchar>(uri[index + 1])) && std::isxdigit(static_cast<Uchar>(uri[index + 2])))
					{
						decoded += static_cast<Char>(std::stoi(uri.substr(index + 1, 2), nullptr, 16));
						index += 2;
					}
					else
					{
						decoded += uri[index];
					}
				}

				std::filesystem::path companion = (logical.parent_path() / std::filesystem::path(std::u8string(decoded.begin(), decoded.end()))).lexically_normal();
				if (companion.empty() || *companion.begin() == "..")
				{
					continue;
				}

				String path = String(companion.generic_wstring());
				if (!std::ranges::contains(companions, path))
				{
					companions.push_back(path);
				}
			}
		}
		return companions;
	}

	std::filesystem::path SharingWorker::Local(const String& logicalPath)const
	{
		return projectRoot_ / config_.workspace_.str() / logicalPath.w_str();
	}

	void SharingWorker::Capture()
	{
		DynamicArray<SharedProgress> progress;
		for (const std::pair<const String, WorkspaceRecord>& record : workspace_)
		{
			const SharedAsset* asset = catalog_.Find(record.first);
			if (!asset)
			{
				continue;
			}

			Bool modified = false;
			for (const WorkspaceFile& file : record.second.files_)
			{
				std::error_code errorCode;
				std::filesystem::path local = Local(file.path_);

				Uint64 size = std::filesystem::file_size(local, errorCode);
				Int64 stamp = std::filesystem::last_write_time(local, errorCode).time_since_epoch().count();
				if (!errorCode && size == file.size_ && stamp == file.stamp_)
				{
					continue;
				}

				if (!std::filesystem::exists(local))
				{
					continue;
				}

				DynamicArray<Byte> digest = Sha256::Hash(local);
				std::string current;
				for (Byte value : digest)
				{
					current += std::format("{:02x}", static_cast<Uint8>(value));
				}
				modified = modified || current != file.hash_.str();
			}

			Bool behind = record.second.revision_ < asset->revision_;
			if (!modified && !behind)
			{
				continue;
			}

			SharedProgress entry;
			entry.assetId_ = record.first;
			entry.modified_ = modified;

			entry.conflicted_ = modified && behind;
			progress.push_back(entry);
		}

		std::lock_guard<std::mutex> guard(snapshotMutex_);
		snapshot_.assets_ = catalog_.Assets();
		snapshot_.progress_ = progress;

		++snapshot_.revision_;
	}

	void SharingWorker::LoadWorkspace()
	{
		std::ifstream stream(projectRoot_ / ".asset" / "workspace.json");
		if (!stream)
		{
			return;
		}

		nlohmann::json workspace = nlohmann::json::parse(stream, nullptr, false);
		if (!workspace.is_object() || !workspace.contains("assets"))
		{
			return;
		}

		for (auto& entry : workspace["assets"].items())
		{
			WorkspaceRecord record;
			record.revision_ = entry.value().value("revision", Uint64(0));

			for (const nlohmann::json& file : entry.value()["files"])
			{
				WorkspaceFile stored;
				stored.path_ = String(file.value("path", ""));
				stored.hash_ = String(file.value("hash", ""));
				stored.size_ = file.value("size", Uint64(0));
				stored.stamp_ = file.value("stamp", Int64(0));
				record.files_.push_back(stored);
			}
			workspace_[String(entry.key())] = record;
		}

		if (!workspace.contains("scenes") || !workspace["scenes"].is_object())
		{
			return;
		}
		for (auto& entry : workspace["scenes"].items())
		{
			SceneBase base;
			for (auto& part : entry.value().items())
			{
				base.parts_[String(part.key())] = ScenePartBase{ part.value().value("revision", Uint64(0)), String(part.value().value("hash", "")) };
			}
			sceneBases_[String(entry.key())] = base;
		}
	}

	void SharingWorker::SaveWorkspace()const
	{
		nlohmann::json workspace;
		workspace["version"] = 1;
		workspace["assets"] = nlohmann::json::object();
		for (const std::pair<const String, WorkspaceRecord>& entry : workspace_)
		{
			nlohmann::json files = nlohmann::json::array();
			for (const WorkspaceFile& file : entry.second.files_)
			{
				files.push_back({ { "path", file.path_.str() }, { "hash", file.hash_.str() }, { "size", file.size_ }, { "stamp", file.stamp_ } });
			}
			workspace["assets"][entry.first.str()] = { { "revision", entry.second.revision_ }, { "files", files } };
		}

		workspace["scenes"] = nlohmann::json::object();
		for (const std::pair<const String, SceneBase>& entry : sceneBases_)
		{
			nlohmann::json parts = nlohmann::json::object();
			for (const std::pair<const String, ScenePartBase>& part : entry.second.parts_)
			{
				parts[part.first.str()] = { { "revision", part.second.revision_ }, { "hash", part.second.hash_.str() } };
			}
			workspace["scenes"][entry.first.str()] = parts;
		}

		std::error_code errorCode;
		std::filesystem::create_directories(projectRoot_ / ".asset", errorCode);
		std::ofstream stream(projectRoot_ / ".asset" / "workspace.json");
		if (stream)
		{
			stream << workspace.dump(1, '\t');
		}
	}
}
