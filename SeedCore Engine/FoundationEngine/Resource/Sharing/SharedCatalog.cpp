#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/SharedCatalog.h>

namespace SeedCore
{
	static void AdoptCatalog(const nlohmann::json& catalog, DynamicArray<SharedAsset>& assets);

	template<typename Edit>
	static Bool ModifyCatalog(GoogleDocument& document, const String& documentId, DynamicArray<SharedAsset>& assets, String& error, Edit&& edit);

	SharedCatalog::SharedCatalog(GoogleDocument& document, const String& documentId) : document_(document), documentId_(documentId)
	{
		/// No Code
	}

	SharedCatalog::~SharedCatalog()
	{
		/// No Code
	}

	Bool SharedCatalog::Refresh()
	{
		GoogleDocumentSnapshot snapshot = document_.Read(documentId_);
		if (!snapshot.valid_)
		{
			error_ = document_.Error();
			return false;
		}

		nlohmann::json catalog = nlohmann::json::parse(snapshot.text_.str(), nullptr, false);
		AdoptCatalog(catalog, assets_);
		error_ = String();
		return true;
	}

	const DynamicArray<SharedAsset>& SharedCatalog::Assets()const
	{
		return assets_;
	}

	const SharedAsset* SharedCatalog::Find(const String& assetId)const
	{
		for (const SharedAsset& asset : assets_)
		{
			if (asset.id_ == assetId)
			{
				return &asset;
			}
		}
		return nullptr;
	}

	const SharedAsset* SharedCatalog::Find(Uint32 runtimeId)const
	{
		for (const SharedAsset& asset : assets_)
		{
			if (asset.runtimeId_ == runtimeId)
			{
				return &asset;
			}
		}
		return nullptr;
	}

	const SharedAsset* SharedCatalog::FindPath(const String& path)const
	{
		for (const SharedAsset& asset : assets_)
		{
			if (asset.path_ == path)
			{
				return &asset;
			}
		}
		return nullptr;
	}

	Bool SharedCatalog::Register(const SharedAsset& asset)
	{
		return ModifyCatalog(document_, documentId_, assets_, error_, [this, asset](nlohmann::json& catalog)
		{
			for (auto& entry : catalog["assets"].items())
			{
				Bool samePath = entry.value().value("path", "") == asset.path_.str();
				Bool sameRuntimeId = entry.value().value("runtimeId", Uint32(0)) == asset.runtimeId_;

				if (samePath || sameRuntimeId)
				{
					error_ = String(std::format("\"{}\" は既に共有されています。", entry.value().value("path", "")));
					return false;
				}
			}

			nlohmann::json files = nlohmann::json::array();
			for (const SharedFile& file : asset.files_)
			{
				files.push_back({ { "path", file.path_.str() }, { "hash", file.hash_.str() }, { "driveId", file.driveId_.str() }, { "size", file.size_ } });
			}

			nlohmann::json dependencies = nlohmann::json::array();
			for (const String& dependency : asset.dependencies_)
			{
				dependencies.push_back(dependency.str());
			}

			nlohmann::json entry;
			entry["path"] = asset.path_.str();
			entry["runtimeId"] = asset.runtimeId_;
			entry["type"] = asset.type_;
			entry["revision"] = 1;
			entry["files"] = files;
			entry["dependencies"] = dependencies;
			entry["scene"] = asset.scene_;
			entry["sceneDocumentId"] = asset.sceneDocumentId_.str();
			catalog["assets"][asset.id_.str()] = entry;
			return true;
		});
	}

	Bool SharedCatalog::Publish(const String& assetId, const DynamicArray<SharedFile>& files, const DynamicArray<String>& dependencies, Uint64 baseRevision, Uint64& revision)
	{
		return ModifyCatalog(document_, documentId_, assets_, error_, [this, assetId, files, dependencies, baseRevision, &revision](nlohmann::json& catalog)
		{
			if (!catalog["assets"].contains(assetId.str()))
			{
				error_ = String("そのアセットは共有ライブラリにありません。");
				return false;
			}
			nlohmann::json& entry = catalog["assets"][assetId.str()];

			if (entry.value("revision", Uint64(0)) != baseRevision)
			{
				error_ = String("他のメンバーが先に公開しているため公開しませんでした。「最新を取得」で取り込んでから編集し直してください。");
				return false;
			}

			nlohmann::json list = nlohmann::json::array();
			for (const SharedFile& file : files)
			{
				list.push_back({ { "path", file.path_.str() }, { "hash", file.hash_.str() }, { "driveId", file.driveId_.str() }, { "size", file.size_ } });
			}
			entry["files"] = list;

			nlohmann::json links = nlohmann::json::array();
			for (const String& dependency : dependencies)
			{
				links.push_back(dependency.str());
			}
			entry["dependencies"] = links;

			revision = entry.value("revision", Uint64(0)) + 1;
			entry["revision"] = revision;
			return true;
		});
	}

	Bool SharedCatalog::Remove(const String& assetId)
	{
		return ModifyCatalog(document_, documentId_, assets_, error_, [this, assetId](nlohmann::json& catalog)
		{
			if (!catalog["assets"].contains(assetId.str()))
			{
				error_ = String("そのアセットは共有ライブラリにありません。");
				return false;
			}
			catalog["assets"].erase(assetId.str());
			return true;
		});
	}

	const String& SharedCatalog::Error()const
	{
		return error_;
	}

	template<typename Edit>
	static Bool ModifyCatalog(GoogleDocument& document, const String& documentId, DynamicArray<SharedAsset>& assets, String& error, Edit&& edit)
	{
		for (Int attempt = 0; attempt < 5; ++attempt)
		{
			GoogleDocumentSnapshot snapshot = document.Read(documentId);
			if (!snapshot.valid_)
			{
				error = document.Error();
				return false;
			}

			nlohmann::json catalog = nlohmann::json::parse(snapshot.text_.str(), nullptr, false);
			if (!catalog.is_object())
			{
				catalog = nlohmann::json::object();
			}
			catalog["version"] = 1;
			if (!catalog.contains("assets") || !catalog["assets"].is_object())
			{
				catalog["assets"] = nlohmann::json::object();
			}

			for (auto entry = catalog["assets"].begin(); entry != catalog["assets"].end();)
			{
				entry = entry->value("deleted", false) ? catalog["assets"].erase(entry) : std::next(entry);
			}

			if (!edit(catalog))
			{
				AdoptCatalog(catalog, assets);
				return false;
			}

			if (document.Write(documentId, snapshot, String(catalog.dump())))
			{
				AdoptCatalog(catalog, assets);
				error = String();
				return true;
			}

			if (!document.Conflicted())
			{
				error = document.Error();
				return false;
			}
		}

		error = String("共有カタログが混み合っています。少し待ってからやり直してください。");
		return false;
	}

	static void AdoptCatalog(const nlohmann::json& catalog, DynamicArray<SharedAsset>& assets)
	{
		assets.clear();
		if (!catalog.is_object() || !catalog.contains("assets") || !catalog["assets"].is_object())
		{
			return;
		}

		for (auto& entry : catalog["assets"].items())
		{
			if (entry.value().value("deleted", false))
			{
				continue;
			}

			SharedAsset asset;
			asset.id_ = String(entry.key());
			asset.path_ = String(entry.value().value("path", ""));
			asset.runtimeId_ = entry.value().value("runtimeId", Uint32(0));
			asset.type_ = entry.value().value("type", Int32(0));
			asset.revision_ = entry.value().value("revision", Uint64(0));
			asset.scene_ = entry.value().value("scene", false);
			asset.sceneDocumentId_ = String(entry.value().value("sceneDocumentId", ""));

			if (entry.value().contains("files"))
			{
				for (const nlohmann::json& file : entry.value()["files"])
				{
					SharedFile shared;
					shared.path_ = String(file.value("path", ""));
					shared.hash_ = String(file.value("hash", ""));
					shared.driveId_ = String(file.value("driveId", ""));
					shared.size_ = file.value("size", Uint64(0));
					asset.files_.push_back(shared);
				}
			}

			if (entry.value().contains("dependencies"))
			{
				for (const nlohmann::json& dependency : entry.value()["dependencies"])
				{
					asset.dependencies_.push_back(String(dependency.get<std::string>()));
				}
			}
			assets.push_back(asset);
		}
	}
}
