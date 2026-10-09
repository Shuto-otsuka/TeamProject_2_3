#include <FoundationEngine/Resource/Sharing/SharedScene.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the document holding one shared scene's pieces.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有 Scene 1つ分の断片を保持しているドキュメントを受け取る。
	*/
	SharedScene::SharedScene(GoogleDocument& document, const String& documentId) : document_(document), documentId_(documentId)
	{
		/// No Code
	}

	/**
	* [EN]
	* Drops the in-memory copy; the pieces themselves live in the
	* document.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メモリ上の写しを捨てるだけ。断片自体はドキュメントにある。
	*/
	SharedScene::~SharedScene()
	{
		/// [EN] A scene closed without publishing leaves the document untouched, and the local edits stay on disk.
		/// [JP] Publish せずに Scene を閉じてもドキュメントは変わらず、ローカルの編集はディスクに残る。

		/// No Code
	}

	/**
	* [EN]
	* Re-reads the scene so the Editor sees which pieces other members
	* have moved on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Scene を読み直し、他のメンバーがどの断片を進めたのかを Editor へ
	* 反映する。
	*/
	Bool SharedScene::Refresh()
	{
		GoogleDocumentSnapshot snapshot = document_.Read(documentId_);
		if (!snapshot.valid_)
		{
			error_ = document_.Error();
			return false;
		}

		/// [EN] A scene that has never been published reads as an empty document, which is simply a scene with no pieces yet.
		/// [JP] 一度も Publish されていない Scene は空のドキュメントとして読める。断片がまだ無い Scene というだけのこと。
		nlohmann::json scene = nlohmann::json::parse(snapshot.text_.str(), nullptr, false);
		Adopt(scene);
		error_ = String();
		return true;
	}

	/**
	* [EN]
	* Every piece of the scene as of the last read, removed ones
	* included.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の読み取り時点における Scene の全断片。削除済みのものも
	* 含む。
	*/
	const DynamicArray<ScenePart>& SharedScene::Parts()const
	{
		return parts_;
	}

	/**
	* [EN]
	* Looks one piece up by its scope, or nullptr when the scene has no
	* such piece.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 範囲を指定して断片を1つ引く。該当が無ければ nullptr。
	*/
	const ScenePart* SharedScene::Find(const String& scope)const
	{
		/// [EN] A scene holds a few pieces per entity plus two more, so a straight scan stays cheap even for a large scene.
		/// [JP] Scene が持つ断片は Entity ごとに数個と、それに2つ足した程度なので、大きな Scene でも素直な走査で足りる。
		for (const ScenePart& part : parts_)
		{
			if (part.scope_ == scope)
			{
				return &part;
			}
		}
		return nullptr;
	}

	/**
	* [EN]
	* Publishes several pieces at once in one write, replacing whatever
	* the library held for each: the last publish wins. Every piece
	* moves one revision past what the library had.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 複数の断片を1回の書き込みでまとめて Publish し、ライブラリが持って
	* いた各断片を置き換える。最後に Publish したものが優先される。各断片
	* は、ライブラリにあった Revision の1つ先へ進む。
	*/
	Bool SharedScene::Publish(const DynamicArray<ScenePartChange>& changes)
	{
		if (changes.empty())
		{
			/// [EN] Saving a scene without touching anything is normal, and there is then nothing to tell the team.
			/// [JP] 何も触らずに Scene を保存することは普通にあり、その場合チームへ伝えることは何も無い。
			return true;
		}

		return Modify([this, changes](nlohmann::json& scene)
		{
			for (const ScenePartChange& change : changes)
			{
				/// [EN] The revision is read from what the library holds right now, so it still moves forward when someone published in between.
				/// [JP] Revision は今ライブラリにある値から読む。間に誰かが Publish していても、番号は前へ進む。
				Uint64 current = 0;
				if (scene["parts"].contains(change.scope_.str()))
				{
					current = scene["parts"][change.scope_.str()].value("revision", Uint64(0));
				}

				nlohmann::json part;
				part["revision"] = current + 1;
				part["hash"] = change.hash_.str();

				/// [EN] A small piece rides inside the document, which is what makes a scene edit arrive in one read rather than two.
				/// [JP] 小さい断片はドキュメントの中に同乗させる。Scene の変更が2回ではなく1回の読み取りで届くのはこのため。
				part["data"] = change.data_.str();
				part["driveId"] = change.driveId_.str();
				part["size"] = change.size_;

				/// [EN] A removed piece keeps its entry and its revision, so a member with an old copy learns it was deleted.
				/// [JP] 削除された断片も項目と Revision を保つ。古い写しを持つメンバーが、削除されたと知るために要る。
				part["deleted"] = change.deleted_;
				scene["parts"][change.scope_.str()] = part;
			}
			return true;
		});
	}

	/**
	* [EN]
	* The last failure, in a form that can be shown to the user.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の失敗内容。ユーザーへそのまま表示できる形で返す。
	*/
	const String& SharedScene::Error()const
	{
		return error_;
	}

	/**
	* [EN]
	* Reads the scene, applies edit to it, and writes it back,
	* retrying from the fresh contents whenever another member wrote
	* first.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Scene を読み、edit を適用して書き戻す。他のメンバーが先に書いて
	* いた場合は、新しい内容から読み直してやり直す。
	*/
	Bool SharedScene::Modify(const std::function<Bool(nlohmann::json&)>& edit)
	{
		/// [EN] Members in one scene write to this same document, so losing a round here is expected rather than rare.
		/// [JP] 同じ Scene にいるメンバーはこの同じドキュメントへ書くので、ここで1周負けるのは珍しくない。
		for (Int attempt = 0; attempt < 5; ++attempt)
		{
			GoogleDocumentSnapshot snapshot = document_.Read(documentId_);
			if (!snapshot.valid_)
			{
				error_ = document_.Error();
				return false;
			}

			/// [EN] The document is put into a known shape first, so the first publish of a scene starts from no pieces.
			/// [JP] まず決まった形に整えるため、Scene の最初の Publish でも断片が無い状態から始められる。
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

			/// [EN] A refused edit still leaves the freshly read scene in memory, which is what the recovery flow looks at.
			/// [JP] 拒否された場合も、読み直した Scene はメモリに残る。Recovery の処理が見るのはそれ。
			if (!edit(scene))
			{
				Adopt(scene);
				return false;
			}

			if (document_.Write(documentId_, snapshot, String(scene.dump())))
			{
				Adopt(scene);
				error_ = String();
				return true;
			}

			/// [EN] Losing the document's own revision only means another piece was published at the same moment.
			/// [JP] ドキュメント自体の版で負けたのは、同じ瞬間に別の断片が Publish されたというだけのこと。
			if (!document_.Conflicted())
			{
				error_ = document_.Error();
				return false;
			}
		}

		error_ = String("この Scene は今、多くのメンバーが同時に編集しています。少し待ってからやり直してください。");
		return false;
	}

	/**
	* [EN]
	* Parses the document text into the in-memory piece list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ドキュメントの本文を解釈して、メモリ上の断片一覧にする。
	*/
	void SharedScene::Adopt(const nlohmann::json& scene)
	{
		/// [EN] The list is rebuilt from scratch, so a piece removed by someone else disappears from this Editor too.
		/// [JP] 一覧は毎回作り直す。他の誰かが削除した断片は、この Editor からも消える。
		parts_.clear();
		if (!scene.is_object() || !scene.contains("parts") || !scene["parts"].is_object())
		{
			return;
		}

		/// [EN] items() is used because the scope is the key rather than a field inside the entry.
		/// [JP] 範囲が項目の中ではなく見出しの側にあるため、items() を使う。
		for (auto& entry : scene["parts"].items())
		{
			ScenePart part;
			part.scope_ = String(entry.key());
			part.revision_ = entry.value().value("revision", Uint64(0));
			part.hash_ = String(entry.value().value("hash", ""));

			/// [EN] A piece written into the document arrives with this read, and one kept in Drive leaves this empty.
			/// [JP] ドキュメントに書かれた断片はこの読み取りで一緒に届き、Drive に置かれた断片ではここが空になる。
			part.data_ = String(entry.value().value("data", ""));
			part.driveId_ = String(entry.value().value("driveId", ""));
			part.size_ = entry.value().value("size", Uint64(0));
			part.deleted_ = entry.value().value("deleted", false);
			parts_.push_back(part);
		}
	}
}
