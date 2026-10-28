#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>

namespace SeedCore
{
	/**
	* [EN]
	* Fallback: false for any T lacking an is_component_behaviour_tag member
	* typedef (i.e. T doesn't derive from ComponentBehaviour).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォールバック: is_component_behaviour_tag というメンバ型定義を持たない
	* 任意の T に対して false になる（すなわち T が ComponentBehaviour から
	* 派生していない）。
	*/
	template<typename T, typename = void>
	struct HasComponentBehaviourTag : std::false_type {};

	/**
	* [EN]
	* Specialization: true when T::is_component_behaviour_tag exists (i.e. T
	* derives from ComponentBehaviour).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 特殊化: T::is_component_behaviour_tag が存在する場合に true になる
	* （すなわち T が ComponentBehaviour から派生している）。
	*/
	template<typename T>
	struct HasComponentBehaviourTag<T, std::void_t<typename T::is_component_behaviour_tag>> : std::true_type {};

	/**
	* [EN]
	* Per-type statics for component type T: the cached ComponentID, and the
	* two tables a ComponentRecord points at. Each module that names T gets
	* its own copy, and the tables live inside that module's image, so the
	* address of a table also tells which module registered it. Both tables
	* are constant-initialized, so they are ready before any static
	* registration runs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネント型 T ごとの静的なもの: キャッシュした ComponentID と、
	* ComponentRecord が指す2つの表。T を使う各モジュールがそれぞれ自分の
	* 写しを持ち、表はそのモジュールのイメージの中に置かれるので、表の
	* アドレスからどのモジュールが登録したかも分かる。2つの表は定数初期化
	* されるので、どの静的登録が走るより前に用意できている。
	*/
	template<typename T>
	struct ComponentType
	{
		/// [EN] T's ComponentID in this module, filled when Register<T> runs here; nullptr until then. T's header always carries REGISTER_COMPONENT, so every module that uses T fills it during static initialization.
		/// [JP] このモジュールでの T の ComponentID。ここで Register<T> が走ったときに入り、それまでは nullptr。T のヘッダには必ず REGISTER_COMPONENT があるので、T を使うモジュールはどれも静的初期化の間に埋める。
		static inline ComponentID id_ = nullptr;

		/// [EN] How T's data is laid out and handled.
		/// [JP] T のデータの並び方と扱い方。
		SC_CONSTEX ComponentMetadata metadata_ = Component<T>::Metadata();

		/// [EN] How T behaves; filled only for ComponentBehaviour-derived T (for any other T every entry stays nullptr and Register<T> does not point at it), and each hook is bound only when T implements it.
		/// [JP] T の振る舞い。ComponentBehaviour 派生の T でだけ中身が埋まる（それ以外の T では全項目が nullptr のままで、Register<T> もこれを指さない）。各フックは T が実装しているときだけ束縛する。
		SC_CONSTEX ComponentHooks hooks_ = []()
			{
				ComponentHooks hooks;

				/// [EN] Every cast below goes from ComponentBehaviour to T, which only compiles when T derives from it.
				/// [JP] 以下の変換はどれも ComponentBehaviour から T へのもので、T がその派生であるときにしかコンパイルできない。
				if constexpr (HasComponentBehaviourTag<T>::value)
				{
					/// [EN] The cast goes through T, so it lands on the ComponentBehaviour base wherever it sits inside T.
					/// [JP] T を経由して変換するので、ComponentBehaviour 基底が T の中のどこにあっても正しく指す。
					hooks.upcast_ = [](void* component) -> ComponentBehaviour* { return static_cast<T*>(component); };

					if constexpr (HasAttach<T>)
					{
						hooks.attach_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnAttach();
							};
					}
					if constexpr (HasDetach<T>)
					{
						hooks.detach_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnDetach();
							};
					}
					if constexpr (HasAwake<T>)
					{
						hooks.awake_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnAwake();
							};
					}
					if constexpr (HasStart<T>)
					{
						hooks.start_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnStart();
							};
					}
					if constexpr (HasTick<T>)
					{
						hooks.tick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnTick(elapsedTime);
							};
					}
					if constexpr (HasFixedTick<T>)
					{
						hooks.fixedTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnFixedTick(elapsedTime);
							};
					}
					if constexpr (HasLateTick<T>)
					{
						hooks.lateTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnLateTick(elapsedTime);
							};
					}
					if constexpr (HasEditorTick<T>)
					{
						hooks.editorTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnEditorTick(elapsedTime);
							};
					}
					if constexpr (HasDestroy<T>)
					{
						hooks.destroy_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnDestroy();
							};
					}
					if constexpr (HasInspectorGUI<T>)
					{
						hooks.inspectorGUI_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnInspectorGUI();
							};
					}
					if constexpr (HasCollisionEnter<T>)
					{
						hooks.collisionEnter_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionEnter(other);
							};
					}
					if constexpr (HasCollisionStay<T>)
					{
						hooks.collisionStay_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionStay(other);
							};
					}
					if constexpr (HasCollisionExit<T>)
					{
						hooks.collisionExit_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionExit(other);
							};
					}
					if constexpr (HasTriggerEnter<T>)
					{
						hooks.triggerEnter_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerEnter(other);
							};
					}
					if constexpr (HasTriggerStay<T>)
					{
						hooks.triggerStay_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerStay(other);
							};
					}
					if constexpr (HasTriggerExit<T>)
					{
						hooks.triggerExit_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerExit(other);
							};
					}
					/// [EN] Bound only for T that rebuilds itself when a field marked SC_REFLECTION_FIELD_REVISION() changes.
					/// [JP] SC_REFLECTION_FIELD_REVISION() の付いたフィールドが変わったときに自身を作り直す T でだけ束縛する。
					if constexpr (HasRevision<T>)
					{
						hooks.revision_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnRevision();
							};
					}
				}
				return hooks;
			}();
	};

	/**
	* [EN]
	* Process-wide registry of every component type known to the ECS. Each
	* type has one ComponentRecord, found by name, which is created on first
	* registration and never freed: unregistering only clears its tables, so
	* the ComponentID and the dense index survive a hot reload unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ECS が認識する全コンポーネント型の、プロセス全体で共有される
	* レジストリ。各型は名前で引ける ComponentRecord を1つ持ち、それは
	* 最初の登録で作られ、解放されない。登録の解除は表を空にするだけ
	* なので、ComponentID と密な添字はホットリロードをまたいでも変わらない。
	*/
	class SEEDCORE_API ComponentRegistry
	{
	public:
		/**
		* [EN]
		* Registers T under name and category, pointing its record at
		* ComponentType<T>'s tables (the hook table only when T derives from
		* ComponentBehaviour). T's storage strategy is ComponentTraits<T>'s.
		* Caches the resulting ID for this module, which only this typed
		* overload can do.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T を name と category で登録し、そのレコードを ComponentType<T> の
		* 表へ向ける（フックの表は T が ComponentBehaviour 派生のときだけ）。
		* T の格納方式は ComponentTraits<T> のもの。得られた ID をこの
		* モジュールでキャッシュする。これは型を知っているこの版にしか
		* できない。
		*/
		template<typename T>
		static void Register(String name, String category = String::intern("Custom"))
		{
			/// [EN] A behaviour must stay at one address while it exists (coroutines hold its this), which archetype rows do not guarantee, so declaring one Archetype is refused at compile time.
			/// [JP] ビヘイビアは存在する間ずっと同じアドレスにいる必要があり（コルーチンが this を持つ）、Archetype の行はそれを保証しないので、Archetype と宣言したらコンパイル時に止める。
			static_assert(!(HasComponentBehaviourTag<T>::value && ComponentTraits<T>::storage == ComponentStorage::Archetype), "ComponentBehaviour は Archetype に格納できません。SparseSet にしてください。");

			/// [EN] Plain-data types have no hook table, so their record's hooks_ stays nullptr.
			/// [JP] ただのデータの型にはフックの表が無いので、そのレコードの hooks_ は nullptr のまま。
			const ComponentHooks* hooks = nullptr;
			if constexpr (HasComponentBehaviourTag<T>::value)
			{
				hooks = &ComponentType<T>::hooks_;
			}

			/// [EN] A name clash leaves nothing to cache; any other outcome, including another module already owning T, hands back T's record.
			/// [JP] 名前がぶつかった場合はキャッシュするものが無い。それ以外は、別のモジュールが既に T を持っている場合も含めて、T のレコードが返る。
			ComponentID id = Register(name, category, &ComponentType<T>::metadata_, hooks);
			if (!id)
			{
				return;
			}

			ComponentType<T>::id_ = id;
		}

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
		static ComponentID Register(String name, String category, const ComponentMetadata* metadata, const ComponentHooks* hooks);

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
		static void Unregister(ComponentID id);

		/**
		* [EN]
		* Returns T's ComponentID, or nullptr if T is not registered. The ID
		* is the one cached when REGISTER_COMPONENT registered T in this
		* module; since records are never freed, the cache never dangles, and
		* the registration is checked on every call.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T の ComponentID を返す。T が未登録であれば nullptr を返す。ID は、
		* このモジュールで REGISTER_COMPONENT が T を登録したときにキャッシュ
		* されたもの。レコードは解放されないのでキャッシュが宙に浮くことは
		* なく、登録中かどうかは毎回確かめる。
		*/
		template<typename T>
		static ComponentID GetComponentID()
		{
			/// [EN] Not yet registered in this module, or unregistered since; either way T has no live registration to hand out.
			/// [JP] このモジュールでまだ登録されていないか、その後に登録が解除された。どちらも、渡せる登録が T に無い。
			if (!ComponentType<T>::id_ || !ComponentType<T>::id_->metadata_)
			{
				return nullptr;
			}
			return ComponentType<T>::id_;
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
		static ComponentID GetComponentID(String name);

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
		static String Name(ComponentID id);

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
		static std::span<const ComponentID> Registry();

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
		static const ComponentMetadata& Get(ComponentID id);

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
		static Size GetID(ComponentID id);

		/**
		* [EN]
		* Returns the byte size of the component type registered under id.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id で登録されているコンポーネント型のバイトサイズを返す。
		*/
		static Size ComponentSize(ComponentID id);

		/**
		* [EN]
		* Returns the alignment requirement of the component type
		* registered under id.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id で登録されているコンポーネント型のアラインメント要件を返す。
		*/
		static Size ComponentAlignment(ComponentID id);

	private:
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
		static ComponentRecord* ResolveRecord(String name);

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
		static void* ResolveModule(const void* address);

	private:
		/// [EN] Every record ever created, in creation order, 64 to a page so a record's address never moves as more are added; a record's position is its dense index, and unused slots have an empty name. DynamicArray's default state needs no allocation, so it is constant-initialized and ready before any static registration runs.
		/// [JP] これまでに作った全レコード。作った順に並び、1ページ 64 個で持つので、レコードを足しても既存の番地は動かない。レコードの位置がそのまま密な添字で、未使用の枠は名前が空。DynamicArray の初期状態は確保を伴わないので定数初期化され、どの静的登録より前に使える状態になっている。
		static inline DynamicArray<ResourcePtr<StaticArray<ComponentRecord, 64>>> pages_;

		/// [EN] IDs currently registered, in registration order, backing Registry(); constant-initialized for the same reason as pages_.
		/// [JP] 今登録されている ID を登録順に並べたもの。Registry() の裏付け。pages_ と同じ理由で定数初期化される。
		static inline DynamicArray<ComponentID> records_;
	};
}

/**
* [EN]
* When Storage is provided (via __VA_ARGS__), declares
* DeclaredComponentStorage(Type*) beside Type, in Type's own namespace,
* where ComponentTraits finds it by ADL; otherwise expands to nothing. Being
* constexpr, the function is implicitly inline, so the header may be
* included from any number of translation units.
*
* ---------------------------------------------------------------------
*
* [JP]
* Storage が指定されている場合（__VA_ARGS__ 経由）、Type の隣、つまり
* Type 自身の名前空間に DeclaredComponentStorage(Type*) を宣言する。
* ComponentTraits はそれを ADL で見つける。指定が無ければ何も生成しない。
* constexpr なので関数は暗黙に inline で、ヘッダをいくつの翻訳単位から
* 取り込んでもよい。
*/
#define SEED_STORAGE_SPEC(Type, ...) \
	__VA_OPT__(constexpr SeedCore::ComponentStorage DeclaredComponentStorage(Type*) { return __VA_ARGS__; })

/**
* [EN]
* Category is the grouping label for the Add Component panel (e.g.
* "Light" so PointLight/SpotLight/... show under one header). Both
* Category and Storage are optional, defaulting to "Custom" and
* SparseSet when omitted. Storage only feeds ComponentTraits through
* SEED_STORAGE_SPEC; Register<T> reads it from there.
* REGISTER_COMPONENT(Type)
* REGISTER_COMPONENT(Type, "Category")
* REGISTER_COMPONENT(Type, "Category", ComponentStorage::Archetype)
*
* ---------------------------------------------------------------------
*
* [JP]
* Category は Add Component パネルのグルーピングラベル（例: "Light" で
* PointLight/SpotLight/... を1つの見出しの下にまとめる）。Category も
* Storage も省略可能で、省略時はそれぞれ "Custom" / SparseSet になる。
* Storage は SEED_STORAGE_SPEC を通して ComponentTraits に渡すだけで、
* Register<T> はそこから読む。
*/
#define REGISTER_COMPONENT_1(Type) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type)); \
		return true; \
	}()

#define REGISTER_COMPONENT_2(Type, Category) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type), SeedCore::String(Category)); \
		return true; \
	}()

#define REGISTER_COMPONENT_3(Type, Category, Storage) \
	SEED_STORAGE_SPEC(Type, Storage) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type), SeedCore::String(Category)); \
		return true; \
	}()

#define REGISTER_COMPONENT_GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define REGISTER_COMPONENT(...) REGISTER_COMPONENT_GET_MACRO(__VA_ARGS__, REGISTER_COMPONENT_3, REGISTER_COMPONENT_2, REGISTER_COMPONENT_1)(__VA_ARGS__)