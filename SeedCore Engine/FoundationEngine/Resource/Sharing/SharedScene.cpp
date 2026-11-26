#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/SharedScene.h>

namespace SeedCore
{
	static void AdoptScene(const nlohmann::json& scene, DynamicArray<ScenePart>& parts);

	SharedScene::SharedScene(GoogleDocument& document, const String& documentId) : document_(document), documentId_(documentId)
	{
		/// No Code
	}

	SharedScene::~SharedScene()
	{
		/// No Code
	}

	Bool SharedScene::Refresh()
	{
		GoogleDocumentSnapshot snapshot = document_.Read(documentId_);
		if (!snapshot.valid_)
		{
			error_ = document_.Error();
			return false;
		}

		nlohmann::json scene = nlohmann::json::parse(snapshot.text_.str(), nullptr, false);
		AdoptScene(scene, parts_);
		error_ = String();
		return true;
	}

	const DynamicArray<ScenePart>& SharedScene::Parts()const
	{
		return parts_;
	}

	const ScenePart* SharedScene::Find(const String& scope)const
	{
		for (const ScenePart& part : parts_)
		{
			if (part.scope_ == scope)
			{
				return &part;
			}
		}
		return nullptr;
	}

	Bool SharedScene::Publish(const DynamicArray<ScenePartChange>& changes)
	{
		if (changes.empty())
		{
			return true;
		}

		return Modify(changes);
	}

	const String& SharedScene::Error()const
	{
		return error_;
	}

	Bool SharedScene::Modify(const DynamicArray<ScenePartChange>& changes)
	{
		for (Int attempt = 0; attempt < 5; ++attempt)
		{
			GoogleDocumentSnapshot snapshot = document_.Read(documentId_);
			if (!snapshot.valid_)
			{
				error_ = document_.Error();
				return false;
			}

			nlohmann::json scene = nlohmann::json::parse(snapshot.text_.str(), nullptr, false);
			if (!scene.is_object())
			{
				scene = nlohmann::json::object();
			}
			scene["version"] = 1;
			if (!scene.contains("parts") || !scene["parts"].is_object())
			{
				scene["parts"] = nlohmann::json::object();
			}

			for (const ScenePartChange& change : changes)
			{
				Uint64 current = 0;
				if (scene["parts"].contains(change.scope_.str()))
				{
					current = scene["parts"][change.scope_.str()].value("revision", Uint64(0));
				}
				if (current != change.baseRevision_)
				{
					error_ = String(std::format("他のメンバーが先に公開した部分があるため公開しませんでした（例: {}）。「最新を取得」すると、その部分はライブラリの版になります。", change.scope_.str()));
					AdoptScene(scene, parts_);
					return false;
				}
			}

			for (const ScenePartChange& change : changes)
			{
				nlohmann::json part;
				part["revision"] = change.baseRevision_ + 1;
				part["hash"] = change.hash_.str();
				part["data"] = change.data_.str();
				part["driveId"] = change.driveId_.str();
				part["size"] = change.size_;
				part["deleted"] = change.deleted_;
				scene["parts"][change.scope_.str()] = part;
			}

			if (document_.Write(documentId_, snapshot, String(scene.dump())))
			{
				AdoptScene(scene, parts_);
				error_ = String();
				return true;
			}

			if (!document_.Conflicted())
			{
				error_ = document_.Error();
				return false;
			}
		}

		error_ = String("この Scene は今、多くのメンバーが同時に編集しています。少し待ってからやり直してください。");
		return false;
	}

	static void AdoptScene(const nlohmann::json& scene, DynamicArray<ScenePart>& parts)
	{
		parts.clear();
		if (!scene.is_object() || !scene.contains("parts") || !scene["parts"].is_object())
		{
			return;
		}

		for (auto& entry : scene["parts"].items())
		{
			ScenePart part;
			part.scope_ = String(entry.key());
			part.revision_ = entry.value().value("revision", Uint64(0));
			part.hash_ = String(entry.value().value("hash", ""));

			part.data_ = String(entry.value().value("data", ""));
			part.driveId_ = String(entry.value().value("driveId", ""));
			part.size_ = entry.value().value("size", Uint64(0));
			part.deleted_ = entry.value().value("deleted", false);
			parts.push_back(part);
		}
	}
}
