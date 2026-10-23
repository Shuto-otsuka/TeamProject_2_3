#pragma once
#include <FoundationEngine/Utility/ReadWrite.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	/**
	* [EN]
	* Fallback: for a plain (non-Read/Write-wrapped) T, the stripped
	* type is T itself.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォールバック: 素の（Read/Write でラップされていない）T に対しては、
	* 剥がした後の型は T 自身になる。
	*/
	template<typename T>
	struct StripAccess
	{
		/// [EN] The underlying component type, with any access-qualifier wrapper removed.
		/// [JP] アクセス修飾ラッパーを取り除いた、内部のコンポーネント型。
		using Type = T;
	};

	/**
	* [EN]
	* Specialization: strips the Read<T> wrapper down to T.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 特殊化: Read<T> ラッパーを取り除いて T にする。
	*/
	template<typename T>
	struct StripAccess<Read<T>>
	{
		/// [EN] The underlying component type, with the Read<> wrapper removed.
		/// [JP] Read<> ラッパーを取り除いた、内部のコンポーネント型。
		using Type = T;
	};

	/**
	* [EN]
	* Specialization: strips the Write<T> wrapper down to T.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 特殊化: Write<T> ラッパーを取り除いて T にする。
	*/
	template<typename T>
	struct StripAccess<Write<T>>
	{
		/// [EN] The underlying component type, with the Write<> wrapper removed.
		/// [JP] Write<> ラッパーを取り除いた、内部のコンポーネント型。
		using Type = T;
	};

	/// [EN] Convenience alias for StripAccess<T>::Type.
	/// [JP] StripAccess<T>::Type の短縮エイリアス。
	template<typename T>
	using StripAccessType = typename StripAccess<T>::Type;

	/**
	* [EN]
	* Iterates every entity in a World that has all of the component
	* types Ts..., handling archetype-chunked and sparse-set-backed
	* components transparently. When Ts... has at least one
	* archetype-stored type, the matching archetypes' chunks drive the
	* walk; when every type is sparse-stored, the first sparse type's set
	* drives it, so the cost follows how many entities have that
	* component rather than how many entities exist. Which archetypes
	* match, and where each type's column sits in them, is worked out once
	* per Query type and kept: archetypes are never destroyed, so only the
	* ones created since the last walk are ever examined. Ts... may be
	* wrapped in Read<>/Write<> to document (and, via
	* ReadSignature/WriteSignature, expose to a scheduler) which components
	* are read vs written, for dependency/parallelism analysis. Adding or
	* removing components while a walk is in progress asserts, since it
	* moves the rows being iterated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World 内の、コンポーネント型 Ts... を全て持つエンティティを走査する。
	* アーキタイプ・チャンク構造化コンポーネントとスパースセットで
	* 裏付けられたコンポーネントの両方を透過的に扱う。Ts... にアーキタイプ
	* 格納の型が1つでもあれば、一致するアーキタイプのチャンクから回す。
	* 全てがスパース格納なら、最初のスパース格納の型のセットから回すので、
	* 費用は存在するエンティティの数ではなく、そのコンポーネントを持つ
	* エンティティの数に比例する。どのアーキタイプが一致し、各型の列が
	* その中のどこにあるかは、Query の型ごとに一度求めて覚えておく。
	* アーキタイプは破棄されないので、調べるのは前回の走査より後に作られた
	* ものだけになる。Ts... は Read<>/Write<> でラップすることで、どの
	* コンポーネントが読み取り/書き込み対象かを明示できる
	* （ReadSignature/WriteSignature 経由でスケジューラへ公開され、依存関係/
	* 並列性の解析に使われる）。走査の途中でコンポーネントを付け外しすると、
	* 走査中の行が動くのでアサートする。
	*/
	template<typename... Ts>
	class Query
	{
	public:
		/**
		* [EN]
		* Constructs a query over world. Everything that depends only on Ts...
		* is kept per Query type, so construction just looks up world's
		* storages.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world に対するクエリを構築する。Ts... だけで決まるものは Query の
		* 型ごとに覚えているので、構築は world のストレージを引くだけ。
		*/
		explicit Query(World& world) :world_(world), archetypes_(WorldAccessor::Archetypes(world)), sparseSets_(WorldAccessor::SparseSets(world))
		{
			/// No Code
		}

		/**
		* [EN]
		* Invokes function for every entity matching this query, one after
		* another on the calling thread. function may optionally take
		* EntityID as its first parameter.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このクエリに一致する各エンティティに対して、呼び出したスレッドで順に
		* function を呼び出す。function は任意で第一引数に EntityID を
		* 受け取れる。
		*/
		template<typename F>
		void ForEach(F&& function)
		{
			/// [EN] Both storages advance their version on every add and remove, so a version that changes during the walk means the rows being iterated moved.
			/// [JP] 両ストレージは追加・削除のたびに版を進めるので、走査中に版が変われば走査中の行が動いたことを意味する。
			Uint32 archetypeVersion = archetypes_.Version();
			Uint32 sparseVersion = sparseSets_.Version();

			if constexpr (hasArchetype_)
			{
				/// [EN] Only the archetypes known to match are walked; every row of their chunks matches the archetype part.
				/// [JP] 一致すると分かっているアーキタイプだけを回す。そのチャンクの全行がアーキタイプの部分に一致する。
				Refresh();
				for (const Match& match : matches_)
				{
					for (Chunk* chunk : archetypes_.Chunks(match.archetype_))
					{
						VisitChunk(match, chunk, function, archetypeVersion, sparseVersion);
					}
				}
			}
			else
			{
				/// [EN] No set means nothing has added the driving component yet, so nothing matches.
				/// [JP] セットが無いのは、まだ誰も回す側のコンポーネントを追加していないということなので、一致するものは無い。
				SparseSet* set = sparseSets_.Set(Driver());
				if (set)
				{
					VisitSet(*set, 0, set->EntityCount(), function, archetypeVersion, sparseVersion);
				}
			}
		}

		/**
		* [EN]
		* Parallel overload of ForEach: dispatches the work to executor and
		* blocks until all of it finishes. Chunks become one job each; a
		* sparse-driven walk is split into ranges of rows. function must only
		* read/write its own entity's components, must not write shared
		* mutable state without synchronization, and must route any
		* structural change through a per-worker CommandBuffer. Intended for
		* pure per-entity work (skin matrices, instance-data packing).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ForEach の並列版: 処理を executor へ投入し、全て終わるまで
		* ブロックする。チャンクはそれぞれ1つのジョブになり、スパース側から
		* 回す場合は行をいくつかの範囲に分ける。function は自分のエンティティ
		* のコンポーネントのみ読み書きし、共有可変状態を同期なしで書かず、
		* 構造変更はワーカーごとの CommandBuffer 経由にしなければならない。
		* 純 per-entity 計算（スキン行列、インスタンスデータ packing）を想定。
		*/
		template<typename F>
		void ForEach(JobExecutor& executor, F&& function)
		{
			Uint32 archetypeVersion = archetypes_.Version();
			Uint32 sparseVersion = sparseSets_.Version();

			/// [EN] Independent tasks over disjoint rows, so they need no ordering between them; function is captured by reference and shared, hence the per-entity-only rule on the caller.
			/// [JP] 重ならない行に対する独立したタスクなので、間に順序は要らない。function は参照で取り込み全タスクで共有するので、呼び出し側には「自分のエンティティだけ触る」決まりが要る。
			JobTaskflow taskflow;

			if constexpr (hasArchetype_)
			{
				Refresh();
				for (const Match& match : matches_)
				{
					for (Chunk* chunk : archetypes_.Chunks(match.archetype_))
					{
						/// [EN] The match is copied into the task, so a task never refers into matches_ itself.
						/// [JP] 一致の情報はタスクへ写すので、タスクが matches_ の中を指すことは無い。
						taskflow.emplace([this, match, chunk, &function, archetypeVersion, sparseVersion]()
							{
								VisitChunk(match, chunk, function, archetypeVersion, sparseVersion);
							});
					}
				}
			}
			else
			{
				SparseSet* set = sparseSets_.Set(Driver());
				if (set)
				{
					/// [EN] Ranges of 1024 rows, the most a chunk holds, so a sparse-driven job is about as big as an archetype-driven one.
					/// [JP] 範囲は 1024 行で、チャンクが持てる最大と同じ。スパース側から回すジョブも、アーキタイプ側から回すジョブと同じくらいの大きさになる。
					Uint32 count = set->EntityCount();
					for (Uint32 first = 0; first < count; first += 1024)
					{
						Uint32 last = Min(first + 1024, count);
						taskflow.emplace([this, set, first, last, &function, archetypeVersion, sparseVersion]()
							{
								VisitSet(*set, first, last, function, archetypeVersion, sparseVersion);
							});
					}
				}
			}

			/// [EN] Block until every job is done, so the caller sees a finished pass just like the serial ForEach.
			/// [JP] 全ジョブが終わるまで待つ。呼び出し側からは直列の ForEach と同じく、処理が終わった状態で戻る。
			executor.Run(taskflow).wait();
		}

		/**
		* [EN]
		* Returns the combined archetype signature of every
		* archetype-stored component in Ts..., built once per Query type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Ts... のうちアーキタイプ格納の全コンポーネントを合わせた
		* シグネチャを返す。Query の型ごとに一度だけ作る。
		*/
		static const Bitset& Signature()
		{
			/// [EN] Built on first use, after every component has registered.
			/// [JP] 初めて使うとき、つまり全コンポーネントの登録が済んだ後に作る。
			static const Bitset signature = []()
				{
					Bitset built;
					([&]()
						{
							if constexpr (ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::Archetype)
							{
								/// [EN] Same bit numbering as Archetype::Signature(): the component's dense internal ID, growing the bitset as needed.
								/// [JP] Archetype::Signature() と同じビット番号(コンポーネントの密な内部 ID)を使い、足りなければビットセットを伸ばす。
								Size internalID = ComponentRegistry::GetID(ComponentRegistry::GetComponentID<StripAccessType<Ts>>());
								if (internalID >= built.size())
								{
									built.resize(internalID + 1);
								}
								built.set(internalID);
							}
						}(), ...);
					return built;
				}();
			return signature;
		}

		/**
		* [EN]
		* Returns the signature of only the components in Ts... that are
		* wrapped in Read<> (used by a scheduler to detect read-after-write
		* hazards between queries).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Ts... のうち Read<> でラップされているコンポーネントのみの
		* シグネチャを返す（スケジューラがクエリ間の read-after-write
		* ハザードを検出するために使う）。
		*/
		static Bitset ReadSignature()
		{
			Bitset signature;
			([&]()
				{
					/// [EN] Unlike Signature(), sparse-set types are included too: a hazard exists whatever the storage.
					/// [JP] Signature() と違い、スパースセット格納の型も含める。競合は格納方式に関係なく起こるため。
					if constexpr (IsReadAccess<Ts>)
					{
						ComponentID id = ComponentRegistry::GetComponentID<StripAccessType<Ts>>();
						Size internalID = ComponentRegistry::GetID(id);
						if (internalID >= signature.size())
						{
							signature.resize(internalID + 1);
						}
						signature.set(internalID);
					}
				}(), ...);
			return signature;
		}

		/**
		* [EN]
		* Returns the signature of only the components in Ts... that are
		* wrapped in Write<> (used by a scheduler to detect
		* write-after-write/write-after-read hazards between queries).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Ts... のうち Write<> でラップされているコンポーネントのみの
		* シグネチャを返す（スケジューラがクエリ間の
		* write-after-write/write-after-read ハザードを検出するために
		* 使う）。
		*/
		static Bitset WriteSignature()
		{
			Bitset signature;
			([&]()
				{
					/// [EN] Includes sparse-set types for the same reason as ReadSignature().
					/// [JP] ReadSignature() と同じ理由で、スパースセット格納の型も含める。
					if constexpr (IsWriteAccess<Ts>)
					{
						ComponentID id = ComponentRegistry::GetComponentID<StripAccessType<Ts>>();
						Size internalID = ComponentRegistry::GetID(id);
						if (internalID >= signature.size())
						{
							signature.resize(internalID + 1);
						}
						signature.set(internalID);
					}
				}(), ...);
			return signature;
		}

	private:
		/**
		* [EN]
		* One archetype known to match this Query type, with the column of
		* each type of Ts... in it (unused for sparse-stored types).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この Query の型に一致すると分かっているアーキタイプ1つと、その中での
		* Ts... の各型の列（スパース格納の型では使わない）。
		*/
		struct Match
		{
			/// [EN] The matching archetype.
			/// [JP] 一致するアーキタイプ。
			Archetype* archetype_ = nullptr;

			/// [EN] Column of each type of Ts... in archetype_, in the order of Ts....
			/// [JP] archetype_ の中での、Ts... の各型の列。Ts... の順に並ぶ。
			StaticArray<Size, sizeof...(Ts)> columns_{};
		};

		/**
		* [EN]
		* Examines the archetypes created since the last call and adds the
		* matching ones, with their columns, to matches_. Archetypes are only
		* ever added, so a count is enough to know what has been examined.
		* Another thread may walk the same Query type at the same time, so
		* the update is locked and checked again inside the lock.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降に作られたアーキタイプを調べ、一致するものを列と
		* 一緒に matches_ へ加える。アーキタイプは増えるだけなので、どこまで
		* 調べたかは数だけで分かる。同じ Query の型を別のスレッドが同時に
		* 回すことがあるので、更新はロックし、ロックの中でもう一度確かめる。
		*/
		static void Refresh()
		{
			std::span<Archetype* const> archetypes = ArchetypeRegistry::Archetypes();
			if (checked_.load(std::memory_order_acquire) == archetypes.size())
			{
				return;
			}

			std::lock_guard<std::mutex> lock(mutex_);
			Size checked = checked_.load(std::memory_order_relaxed);
			if (checked == archetypes.size())
			{
				return;
			}

			const Bitset& wanted = Signature();
			const Bitset::Block* wantedBlocks = wanted.data();
			Size wantedCount = wanted.block_count();

			for (Size archetypeIndex = checked; archetypeIndex < archetypes.size(); ++archetypeIndex)
			{
				Archetype* archetype = archetypes[archetypeIndex];

				/// [EN] Superset test block by block, without building a temporary bitset: every wanted bit must also be set in the archetype, whose missing blocks count as zero.
				/// [JP] 一時的なビットセットを作らずに、ブロックごとに包含を確かめる。求める全ビットがアーキタイプにも立っている必要があり、アーキタイプに無いブロックは 0 とみなす。
				const Bitset& has = archetype->Signature();
				const Bitset::Block* hasBlocks = has.data();
				Size hasCount = has.block_count();
				Bool matches = true;
				for (Size block = 0; block < wantedCount; ++block)
				{
					Bitset::Block present = (block < hasCount) ? hasBlocks[block] : 0;
					if ((wantedBlocks[block] & present) != wantedBlocks[block])
					{
						matches = false;
						break;
					}
				}

				if (!matches)
				{
					continue;
				}

				/// [EN] The comma fold runs left to right, so position follows the order of Ts....
				/// [JP] カンマによる畳み込みは左から右へ進むので、position は Ts... の順に進む。
				Match match;
				match.archetype_ = archetype;
				Size position = 0;
				((match.columns_[position++] = Column<StripAccessType<Ts>>(archetype)), ...);
				matches_.push_back(match);
			}

			checked_.store(archetypes.size(), std::memory_order_release);
		}

		/**
		* [EN]
		* Returns T's column in archetype, or 0 for a sparse-stored T, which
		* has no column.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archetype の中での T の列を返す。スパース格納の T は列を持たない
		* ので 0。
		*/
		template<typename T>
		static Size Column(Archetype* archetype)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				/// [EN] T's position in the archetype layout is also its sub-array index in every chunk of it.
				/// [JP] アーキタイプのレイアウト内の T の位置が、そのまま各チャンクのサブ配列の番号になる。
				const DynamicArray<ComponentID>& layout = archetype->Layout();
				return static_cast<Size>(std::ranges::find(layout, ComponentRegistry::GetComponentID<T>()) - layout.begin());
			}
			else
			{
				return 0;
			}
		}

		/**
		* [EN]
		* Returns the sparse type whose set drives the walk when Ts... has no
		* archetype-stored type: the first sparse-stored type, found once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Ts... にアーキタイプ格納の型が無いときに、走査を回すセットの
		* スパース格納の型を返す。最初のスパース格納の型を一度だけ求める。
		*/
		static ComponentID Driver()
		{
			static const ComponentID driver = []()
				{
					ComponentID found = nullptr;
					([&]()
						{
							if constexpr (ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::SparseSet)
							{
								if (!found)
								{
									found = ComponentRegistry::GetComponentID<StripAccessType<Ts>>();
								}
							}
						}(), ...);
					return found;
				}();
			return driver;
		}

		/**
		* [EN]
		* Runs function over every matching entity in one chunk: takes each
		* archetype-stored component's column from match, then per-entity
		* resolves any sparse-set components (skipping the entity if any are
		* missing) and invokes function. Shared by both ForEach overloads
		* (one call per chunk).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1つのチャンク内の一致する各エンティティに対して function を
		* 実行する: 各アーキタイプ格納コンポーネントの列を match から取り、
		* エンティティごとにスパースセットコンポーネントを解決して
		* （いずれか欠けていればそのエンティティをスキップ）function を呼ぶ。
		* 両方の ForEach で共有する（チャンクごとに1回呼ぶ）。
		*/
		template<typename F>
		void VisitChunk(const Match& match, Chunk* chunk, F& function, Uint32 archetypeVersion, Uint32 sparseVersion)
		{
			/// [EN] One Resolved per requested type, resolved once for the whole chunk (column base pointers, or sparse-set IDs); the comma fold keeps position in the order of Ts....
			/// [JP] 要求された型ごとに Resolved を1つ持ち、チャンク全体で1回だけ解決する(列の先頭ポインタ、またはスパースセット用の ID)。カンマによる畳み込みで position は Ts... の順に進む。
			std::tuple<Resolved<StripAccessType<Ts>>...> resolved;
			Size position = 0;
			(Resolve<StripAccessType<Ts>>(match.columns_[position++], chunk, std::get<Resolved<StripAccessType<Ts>>>(resolved)), ...);

			/// [EN] Row index doubles as the element index into every archetype column of this chunk.
			/// [JP] 行番号は、このチャンクの全アーキタイプ列に対する要素番号も兼ねる。
			Size count = chunk->EntityCount();
			for (Size row = 0; row < count; ++row)
			{
				EntityID entity = chunk->EntityAt(row);

				/// [EN] Sparse-set types are not guaranteed by the archetype, so each is looked up per entity; && short-circuits on the first miss.
				/// [JP] スパースセット格納の型はアーキタイプでは保証されないので、エンティティごとに引く。&& は最初に欠けた時点で打ち切る。
				Bool hasAllSparse = ([&]()
					{
						if constexpr (ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::SparseSet)
						{
							auto& r = std::get<Resolved<StripAccessType<Ts>>>(resolved);
							r.ptr_ = static_cast<StripAccessType<Ts>*>(sparseSets_.Data(entity, r.id_));
							return r.ptr_ != nullptr;
						}
						else
						{
							/// [EN] Archetype-stored types are present by construction for every row of a matching chunk.
							/// [JP] アーキタイプ格納の型は、一致したチャンクの全行に必ず存在する。
							return true;
						}
					}() && ...);

				/// [EN] Missing any sparse-set component means the entity does not match the query.
				/// [JP] スパースセット格納のコンポーネントが1つでも欠けていれば、そのエンティティはクエリに一致しない。
				if (!hasAllSparse)
				{
					continue;
				}

				/// [EN] If function accepts EntityID as its first parameter, pass it through; otherwise call it the original way. This lets one path serve both call styles.
				/// [JP] function が第一引数として EntityID を受け取れるならそれを渡し、そうでなければ元の呼び方をする。これにより1つの経路で両方の呼び出し形式に対応できる。
				if constexpr (std::is_invocable_v<F&, EntityID, StripAccessType<Ts>&...>)
				{
					function(entity, Fetch<StripAccessType<Ts>>(std::get<Resolved<StripAccessType<Ts>>>(resolved), row)...);
				}
				else
				{
					function(Fetch<StripAccessType<Ts>>(std::get<Resolved<StripAccessType<Ts>>>(resolved), row)...);
				}

				SC_ASSERT(archetypes_.Version() == archetypeVersion && sparseSets_.Version() == sparseVersion, "Query の走査中にコンポーネントの追加/削除が発生しました。走査の後へ遅延させるか、コマンドバッファ経由にしてください。");
			}
		}

		/**
		* [EN]
		* Runs function over rows first to last of the driving sparse set:
		* the driving component comes straight from the row, and every other
		* sparse-set component is resolved per entity (skipping the entity if
		* any are missing). Used when Ts... has no archetype-stored type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 回す側のスパースセットの first 行目から last 行目の手前までに対して
		* function を実行する。回す側のコンポーネントは行からそのまま取り、
		* それ以外のスパースセットコンポーネントはエンティティごとに解決する
		* （いずれか欠けていればそのエンティティをスキップ）。Ts... に
		* アーキタイプ格納の型が無いときに使う。
		*/
		template<typename F>
		void VisitSet(SparseSet& set, Uint32 first, Uint32 last, F& function, Uint32 archetypeVersion, Uint32 sparseVersion)
		{
			/// [EN] Every type is sparse-stored here, so resolving only records each type's ID.
			/// [JP] ここでは全ての型がスパース格納なので、解決は各型の ID を記録するだけ。
			std::tuple<Resolved<StripAccessType<Ts>>...> resolved;
			(Resolve<StripAccessType<Ts>>(0, nullptr, std::get<Resolved<StripAccessType<Ts>>>(resolved)), ...);

			ComponentID driver = Driver();
			for (Uint32 row = first; row < last; ++row)
			{
				EntityID entity = set.EntityAt(row);

				/// [EN] The driving component is already at this row; the others are looked up per entity, and && short-circuits on the first miss.
				/// [JP] 回す側のコンポーネントはこの行にある。それ以外はエンティティごとに引き、&& は最初に欠けた時点で打ち切る。
				Bool hasAll = ([&]()
					{
						auto& r = std::get<Resolved<StripAccessType<Ts>>>(resolved);
						r.ptr_ = static_cast<StripAccessType<Ts>*>(r.id_ == driver ? set.Data(row) : sparseSets_.Data(entity, r.id_));
						return r.ptr_ != nullptr;
					}() && ...);

				if (!hasAll)
				{
					continue;
				}

				if constexpr (std::is_invocable_v<F&, EntityID, StripAccessType<Ts>&...>)
				{
					function(entity, Fetch<StripAccessType<Ts>>(std::get<Resolved<StripAccessType<Ts>>>(resolved), row)...);
				}
				else
				{
					function(Fetch<StripAccessType<Ts>>(std::get<Resolved<StripAccessType<Ts>>>(resolved), row)...);
				}

				SC_ASSERT(archetypes_.Version() == archetypeVersion && sparseSets_.Version() == sparseVersion, "Query の走査中にコンポーネントの追加/削除が発生しました。走査の後へ遅延させるか、コマンドバッファ経由にしてください。");
			}
		}

		/**
		* [EN]
		* Forward declaration selecting a storage-specific Resolved
		* specialization based on T's ComponentTraits.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T の ComponentTraits に基づいて、ストレージ別の Resolved
		* 特殊化を選択するための前方宣言。
		*/
		template<typename T, ComponentStorage Storage = ComponentTraits<T>::storage>
		struct Resolved;

		/**
		* [EN]
		* Resolution state for an archetype-stored component: just the base
		* pointer into the current chunk's sub-array (indexed per-entity by
		* row).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アーキタイプ格納コンポーネント用の解決状態: 現在のチャンクの
		* サブ配列への基底ポインタのみを持つ（エンティティごとに行で
		* インデックスされる）。
		*/
		template<typename T>
		struct Resolved<T, ComponentStorage::Archetype>
		{
			/// [EN] Base pointer into the current chunk's sub-array for T.
			/// [JP] T に対応する、現在のチャンクのサブ配列への基底ポインタ。
			Uint8* base_ = nullptr;
		};

		/**
		* [EN]
		* Resolution state for a sparse-set-stored component: T's
		* ComponentID (resolved once) and a per-entity pointer (resolved
		* fresh for each entity).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スパースセット格納コンポーネント用の解決状態: T の ComponentID
		* （一度だけ解決）と、エンティティごとのポインタ（エンティティ
		* ごとに新たに解決される）。
		*/
		template<typename T>
		struct Resolved<T, ComponentStorage::SparseSet>
		{
			/// [EN] T's registered ComponentID.
			/// [JP] T の登録済み ComponentID。
			ComponentID id_ = nullptr;

			/// [EN] Pointer to the current entity's T instance, refreshed per entity.
			/// [JP] 現在のエンティティの T インスタンスへのポインタ。エンティティごとに更新される。
			T* ptr_ = nullptr;
		};

		/**
		* [EN]
		* Resolves out once per chunk: for archetype storage, caches the base
		* pointer of T's column (already known from the match) in chunk; for
		* sparse-set storage, just caches T's ComponentID (the actual pointer
		* is resolved per entity).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* チャンクごとに1回 out を解決する: アーキタイプストレージの場合、
		* chunk の中の T の列（一致の情報から分かっている）の先頭ポインタを
		* キャッシュする。スパースセットストレージの場合は T の ComponentID
		* のみをキャッシュする（実際のポインタはエンティティごとに解決される）。
		*/
		template<typename T>
		void Resolve(Size column, Chunk* chunk, Resolved<T>& out)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				out.base_ = chunk->Data(column);
			}
			else
			{
				/// [EN] A sparse-set type has no column to cache, only its ID for the per-entity lookup.
				/// [JP] スパースセット格納の型にはキャッシュする列が無いので、エンティティごとの検索に使う ID だけを持つ。
				out.id_ = ComponentRegistry::GetComponentID<T>();
			}
		}

		/**
		* [EN]
		* Returns a reference to the current entity's T from resolved,
		* indexing by row for archetype storage or dereferencing the
		* per-entity pointer for sparse-set storage.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* resolved から現在のエンティティの T への参照を返す。アーキタイプ
		* ストレージの場合は行でインデックスし、スパースセットストレージの
		* 場合はエンティティごとのポインタを逆参照する。
		*/
		template<typename T>
		T& Fetch(Resolved<T>& resolved, Size row)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				/// [EN] The column is a packed T array, so the row picks the element directly.
				/// [JP] 列は T を詰めた配列なので、行でそのまま要素を取り出せる。
				return reinterpret_cast<T*>(resolved.base_)[row];
			}
			else
			{
				/// [EN] Already looked up for this entity before the call.
				/// [JP] 呼ぶ前に、このエンティティ分は既に引いてある。
				return *resolved.ptr_;
			}
		}

	private:
		/// [EN] Whether Ts... has at least one archetype-stored type, which decides whether chunks or a sparse set drive the walk.
		/// [JP] Ts... にアーキタイプ格納の型が1つでもあるか。チャンクとスパースセットのどちらから回すかを決める。
		SC_CONSTEX Bool hasArchetype_ = ((ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::Archetype) || ...);

		/// [EN] Archetypes known to match this Query type, with their columns; shared by every Query of the same Ts... in this module.
		/// [JP] この Query の型に一致すると分かっているアーキタイプと、その列。このモジュールの同じ Ts... の Query 全てで共有する。
		static inline DynamicArray<Match> matches_;

		/// [EN] How many archetypes, counted from the first, have been examined for matches_.
		/// [JP] matches_ のために、先頭から何個のアーキタイプを調べ終えたか。
		static inline std::atomic<Size> checked_ = 0;

		/// [EN] Serializes updates of matches_ when several threads walk the same Query type.
		/// [JP] 同じ Query の型を複数のスレッドが回すときに、matches_ の更新を1つずつにする。
		static inline std::mutex mutex_;

		/// [EN] The World this query iterates over.
		/// [JP] このクエリが走査対象とする World。
		World& world_;

		/// [EN] world_'s archetype storage, looked up once.
		/// [JP] world_ のアーキタイプストレージ。一度だけ引く。
		ArchetypeStorage& archetypes_;

		/// [EN] world_'s sparse set storage, looked up once.
		/// [JP] world_ のスパースセットストレージ。一度だけ引く。
		SparseSetStorage& sparseSets_;
	};
}
