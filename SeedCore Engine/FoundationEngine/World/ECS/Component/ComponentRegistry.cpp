#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [EN]
	* Registers a component under name and category, pointing its record
	* at the given tables, and returns its ComponentID. Takes no type, so
	* it is also how C# scripts register: they share one metadata table
	* and each brings its own hook table.
	*
	* The module that owns a registration is the one whose image holds
	* its metadata table. If a different module that is still loaded
	* already owns name, the call changes nothing and returns that
	* record - the same header compiled into a downstream DLL must not
	* take over the engine's entry. If the same module registers a
	* different pair of tables under a name it already holds, that is
	* two types sharing one name: it is reported and nullptr returned.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネントを name と category で登録し、そのレコードを渡された
	* 表へ向けて、ComponentID を返す。型を受け取らないので、C# の
	* スクリプトもこれで登録する: メタデータの表を共有し、フックの表を
	* それぞれが持ち込む。
	*
	* 登録を所有するのは、メタデータの表をイメージ内に持つモジュール。
	* まだロードされている別のモジュールが既に name を所有していれば、
	* 何も変えずにそのレコードを返す - 下流の DLL に同じヘッダが入っても、
	* エンジンの登録を乗っ取ってはならないため。同じモジュールが、既に
	* 持っている名前で別の表の組を登録したら、それは1つの名前を2つの型が
	* 使っているということ: 報告して nullptr を返す。
	*/
	ComponentID ComponentRegistry::Register(String name, String category, const ComponentMetadata* metadata, const ComponentHooks* hooks)
	{
		/// [EN] An empty name marks an unused slot in pages_, so it can never name a record.
		/// [JP] 空の名前は pages_ の未使用の枠を表すので、レコードの名前には使えない。
		if (name == String())
		{
			SC_LOG_ERROR("名前のないコンポーネントは登録できません。");
			return nullptr;
		}

		if (ResolveModule(metadata) != ResolveModule(reinterpret_cast<const void*>(&ComponentRegistry::ResolveModule)) && category != String("Custum") && !category.str().starts_with("Custum/"))
		{
			category = String("Custum/" + category.str());
		}

		/// [EN] A behaviour must stay at one address while it exists, which archetype rows do not guarantee; types registered without T (C# scripts) are checked here.
		/// [JP] ビヘイビアは存在する間ずっと同じアドレスにいる必要があり、Archetype の行はそれを保証しない。T を伴わずに登録される型（C# のスクリプト）はここで確かめる。
		if (hooks && hooks->upcast_ && metadata && metadata->storage_ == ComponentStorage::Archetype)
		{
			SC_LOG_ERROR("コンポーネント「{}」は ComponentBehaviour なので Archetype に格納できません。", name.str());
			return nullptr;
		}

		ComponentRecord* record = ResolveRecord(name);

		/// [EN] A name seen for the first time gets the first unused slot; its position, counted across pages, becomes its dense index.
		/// [JP] 初めて見る名前には、最初の未使用の枠を割り当てる。ページをまたいで数えたその位置が、密な添字になる。
		if (!record)
		{
			Uint32 index = 0;
			for (const ResourcePtr<StaticArray<ComponentRecord, 64>>& page : pages_)
			{
				for (ComponentRecord& slot : *page)
				{
					if (slot.name_ == String())
					{
						record = &slot;
						break;
					}
					++index;
				}

				if (record)
				{
					break;
				}
			}

			/// [EN] Every page is full: a new page is added, and its first slot sits right after all the counted ones.
			/// [JP] 全ページが埋まっている: 新しいページを足す。その最初の枠は、数えた全ての枠のすぐ後ろにあたる。
			if (!record)
			{
				pages_.push_back(MakePtr<StaticArray<ComponentRecord, 64>>());
				record = &(*pages_.back())[0];
			}

			record->name_ = name;
			record->index_ = index;
		}

		/// [EN] The same tables again: the header was included by one more translation unit of the same module, so only the category is refreshed.
		/// [JP] 同じ表がもう一度来た: 同じモジュールの別の翻訳単位がヘッダを取り込んだだけなので、分類だけを更新する。
		if (record->metadata_ == metadata && record->hooks_ == hooks)
		{
			record->category_ = category;
			return record;
		}

		if (record->metadata_)
		{
			/// [EN] The current owner is the module whose image holds the current metadata table; nullptr means that module is gone.
			/// [JP] 今の所有者は、今のメタデータの表をイメージ内に持つモジュール。nullptr ならそのモジュールはもう無い。
			void* owner = ResolveModule(record->metadata_);
			if (owner)
			{
				/// [EN] Another loaded module owns the name: this is the same header compiled into a downstream DLL, which defers to the owner.
				/// [JP] ロード中の別のモジュールが名前を持っている: 下流の DLL に同じヘッダが入っただけなので、所有者に譲る。
				if (owner != ResolveModule(metadata))
				{
					return record;
				}

				/// [EN] The owning module itself brought different tables under the same name: two types share one name.
				/// [JP] 所有しているモジュール自身が、同じ名前で別の表を持ってきた: 1つの名前を2つの型が使っている。
				SC_LOG_ERROR("コンポーネント「{}」は別の型として既に登録されているため、登録しませんでした。", name.str());
				return nullptr;
			}

			/// [EN] The owner was unloaded without unregistering; the record is still in records_, so it is simply taken over below.
			/// [JP] 所有者が登録を解除しないままアンロードされた。レコードは records_ に残っているので、下でそのまま引き継ぐ。
		}
		else
		{
			/// [EN] The record was unregistered (or just created), so it rejoins the registered list.
			/// [JP] レコードは登録を解除されていた（か、今作った）ので、登録中の一覧に戻す。
			records_.push_back(record);
		}

		record->category_ = category;
		record->metadata_ = metadata;
		record->hooks_ = hooks;
		return record;
	}

	/**
	* [EN]
	* Clears id's tables and advances its epoch, and removes it from the
	* registered list, so nothing points into the owning module once it
	* is unloaded. The record itself, its name and its dense index are
	* kept, so registering the same name again brings back the same
	* ComponentID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id の表を空にして epoch を進め、登録中の一覧から外す。これで、
	* 所有モジュールがアンロードされた後に、その中を指すものが残らない。
	* レコード自体と、その名前と密な添字は残すので、同じ名前で登録し
	* 直すと同じ ComponentID が戻る。
	*/
	void ComponentRegistry::Unregister(ComponentID id)
	{
		/// [EN] Unregistering twice, or a null ID, changes nothing.
		/// [JP] 二重の解除や null の ID では何も変えない。
		if (!id || !id->metadata_)
		{
			return;
		}

		/// [EN] ComponentID is a read-only view of the record, so the writable record is looked up by its name.
		/// [JP] ComponentID はレコードの読み取り専用の見え方なので、書き換えられるレコードを名前で引く。
		ComponentRecord* record = ResolveRecord(id->name_);
		record->metadata_ = nullptr;
		record->hooks_ = nullptr;
		++record->epoch_;

		SeedCore::erase_if(records_, [id](ComponentID registered) { return registered == id; });
	}

	/**
	* [EN]
	* Returns the ComponentID registered under name, or nullptr if
	* no component is currently registered with that name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name で登録された ComponentID を返す。その名前で今登録されている
	* コンポーネントが無ければ nullptr を返す。
	*/
	ComponentID ComponentRegistry::GetComponentID(String name)
	{
		/// [EN] Looking up never creates a record, so a scene naming a type this build does not know leaves the registry untouched.
		/// [JP] 検索ではレコードを作らないので、この実行ファイルが知らない型をシーンが名指ししても、レジストリは変わらない。
		ComponentRecord* record = ResolveRecord(name);
		if (!record || !record->metadata_)
		{
			return nullptr;
		}
		return record;
	}

	/**
	* [EN]
	* Returns the name id was registered under, or an empty string if id
	* is null.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id が登録された名前を返す。id が null であれば空文字列を返す。
	*/
	String ComponentRegistry::Name(ComponentID id)
	{
		if (!id)
		{
			return String();
		}
		return id->name_;
	}

	/**
	* [EN]
	* Returns every currently registered ComponentID. Unregister removes
	* from this very list, so callers that unregister while walking it
	* must collect first.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 今登録されている全ての ComponentID を返す。Unregister はまさに
	* この一覧から消すので、走査しながら登録を解除する呼び出し側は、
	* 先に集めておく必要がある。
	*/
	std::span<const ComponentID> ComponentRegistry::Registry()
	{
		return records_;
	}

	/**
	* [EN]
	* Returns the ComponentMetadata id's record points at; id must be
	* registered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id のレコードが指す ComponentMetadata を返す。id は登録中でなければ
	* ならない。
	*/
	const ComponentMetadata& ComponentRegistry::Get(ComponentID id)
	{
		/// [EN] An unregistered ID has no metadata to return; Debug stops here so the stale ID is caught where it is used.
		/// [JP] 登録を解除された ID には返すメタデータが無い。Debug ではここで止め、古い ID を使った場所で気づけるようにする。
#if SC_DEBUG
		SC_ASSERT(id && id->metadata_);
#endif
		return *id->metadata_;
	}

	/**
	* [EN]
	* Returns id's dense index, assigned at first registration and never
	* changed (used e.g. to index into Archetype signature bitsets).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id の密な添字を返す。最初の登録で振られ、以後変わらない（例えば
	* Archetype のシグネチャビットセットへのインデックス付けに使われる）。
	*/
	Size ComponentRegistry::GetID(ComponentID id)
	{
		return id->index_;
	}

	/**
	* [EN]
	* Returns the byte size of the component type registered under id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id で登録されているコンポーネント型のバイトサイズを返す。
	*/
	Size ComponentRegistry::ComponentSize(ComponentID id)
	{
		return Get(id).size_;
	}

	/**
	* [EN]
	* Returns the alignment requirement of the component type registered
	* under id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id で登録されているコンポーネント型のアラインメント要件を返す。
	*/
	Size ComponentRegistry::ComponentAlignment(ComponentID id)
	{
		return Get(id).alignment_;
	}

	/**
	* [EN]
	* Returns the record named name, or nullptr if none has been created.
	* Records sit in pages_ in creation order and every created record has
	* a name, so the walk stops at the first unnamed slot. Names are
	* interned, so each step is a single pointer comparison.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name という名前のレコードを返す。まだ作られていなければ nullptr。
	* レコードは pages_ に作った順に並び、作ったレコードには必ず名前が
	* あるので、名前の無い最初の枠で探索を止める。名前は intern 済み
	* なので、1歩ごとの比較はポインタ1回で済む。
	*/
	ComponentRecord* ComponentRegistry::ResolveRecord(String name)
	{
		for (const ResourcePtr<StaticArray<ComponentRecord, 64>>& page : pages_)
		{
			for (ComponentRecord& record : *page)
			{
				/// [EN] Every slot past the first unnamed one is unused too, so nothing further can match.
				/// [JP] 名前の無い最初の枠より後ろも全て未使用なので、それ以上は一致しない。
				if (record.name_ == String())
				{
					return nullptr;
				}

				if (record.name_ == name)
				{
					return &record;
				}
			}
		}
		return nullptr;
	}

	/**
	* [EN]
	* Returns the module (HMODULE as an opaque pointer) whose image
	* contains address, or nullptr when no loaded module does.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* address をイメージ内に持つモジュール（HMODULE を不透明なポインタで）
	* を返す。ロード中のどのモジュールも持っていなければ nullptr。
	*/
	void* ComponentRegistry::ResolveModule(const void* address)
	{
		HMODULE module = nullptr;
		if (address)
		{
			/// [EN] FROM_ADDRESS treats the pointer as an address inside a module rather than a name; UNCHANGED_REFCOUNT keeps the lookup from pinning that module in memory.
			/// [JP] FROM_ADDRESS はポインタを名前ではなくモジュール内のアドレスとして扱う。UNCHANGED_REFCOUNT により、検索がそのモジュールをメモリに留めないようにする。
			GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &module);
		}
		return module;
	}
}
