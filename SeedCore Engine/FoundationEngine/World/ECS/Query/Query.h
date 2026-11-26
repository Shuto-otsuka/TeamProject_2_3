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
	template<typename T>
	struct StripAccess
	{
		using Type = T;
	};

	template<typename T>
	struct StripAccess<Read<T>>
	{
		using Type = T;
	};

	template<typename T>
	struct StripAccess<Write<T>>
	{
		using Type = T;
	};

	template<typename T>
	using StripAccessType = typename StripAccess<T>::Type;

	template<typename... Ts>
	class Query
	{
	public:
		explicit Query(World& world) :world_(world), archetypes_(WorldAccessor::Archetypes(world)), sparseSets_(WorldAccessor::SparseSets(world))
		{
			/// No Code
		}

		template<typename F>
		void ForEach(F&& function)
		{
			Uint32 archetypeVersion = archetypes_.Version();
			Uint32 sparseVersion = sparseSets_.Version();

			if constexpr (hasArchetype_)
			{
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
				SparseSet* set = sparseSets_.Set(Driver());
				if (set)
				{
					VisitSet(*set, 0, set->EntityCount(), function, archetypeVersion, sparseVersion);
				}
			}
		}

		template<typename F>
		void ForEach(JobExecutor& executor, F&& function)
		{
			Uint32 archetypeVersion = archetypes_.Version();
			Uint32 sparseVersion = sparseSets_.Version();

			JobTaskflow taskflow;

			if constexpr (hasArchetype_)
			{
				Refresh();
				for (const Match& match : matches_)
				{
					for (Chunk* chunk : archetypes_.Chunks(match.archetype_))
					{
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

			executor.Run(taskflow).wait();
		}

		static const Bitset& Signature()
		{
			static const Bitset signature = []()
				{
					Bitset built;
					([&]()
						{
							if constexpr (ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::Archetype)
							{
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

		static Bitset ReadSignature()
		{
			Bitset signature;
			([&]()
				{
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

		static Bitset WriteSignature()
		{
			Bitset signature;
			([&]()
				{
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
		struct Match
		{
			Archetype* archetype_ = nullptr;

			StaticArray<Size, sizeof...(Ts)> columns_{};
		};

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

				Match match;
				match.archetype_ = archetype;
				Size position = 0;
				((match.columns_[position++] = Column<StripAccessType<Ts>>(archetype)), ...);
				matches_.push_back(match);
			}

			checked_.store(archetypes.size(), std::memory_order_release);
		}

		template<typename T>
		static Size Column(Archetype* archetype)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				const DynamicArray<ComponentID>& layout = archetype->Layout();
				return static_cast<Size>(std::ranges::find(layout, ComponentRegistry::GetComponentID<T>()) - layout.begin());
			}
			else
			{
				return 0;
			}
		}

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

		template<typename F>
		void VisitChunk(const Match& match, Chunk* chunk, F& function, Uint32 archetypeVersion, Uint32 sparseVersion)
		{
			std::tuple<Resolved<StripAccessType<Ts>>...> resolved;
			Size position = 0;
			(Resolve<StripAccessType<Ts>>(match.columns_[position++], chunk, std::get<Resolved<StripAccessType<Ts>>>(resolved)), ...);

			Size count = chunk->EntityCount();
			for (Size row = 0; row < count; ++row)
			{
				EntityID entity = chunk->EntityAt(row);

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
							return true;
						}
					}() && ...);

				if (!hasAllSparse)
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

		template<typename F>
		void VisitSet(SparseSet& set, Uint32 first, Uint32 last, F& function, Uint32 archetypeVersion, Uint32 sparseVersion)
		{
			std::tuple<Resolved<StripAccessType<Ts>>...> resolved;
			(Resolve<StripAccessType<Ts>>(0, nullptr, std::get<Resolved<StripAccessType<Ts>>>(resolved)), ...);

			ComponentID driver = Driver();
			for (Uint32 row = first; row < last; ++row)
			{
				EntityID entity = set.EntityAt(row);

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

		template<typename T, ComponentStorage Storage = ComponentTraits<T>::storage>
		struct Resolved;

		template<typename T>
		struct Resolved<T, ComponentStorage::Archetype>
		{
			Uint8* base_ = nullptr;
		};

		template<typename T>
		struct Resolved<T, ComponentStorage::SparseSet>
		{
			ComponentID id_ = nullptr;

			T* ptr_ = nullptr;
		};

		template<typename T>
		void Resolve(Size column, Chunk* chunk, Resolved<T>& out)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				out.base_ = chunk->Data(column);
			}
			else
			{
				out.id_ = ComponentRegistry::GetComponentID<T>();
			}
		}

		template<typename T>
		T& Fetch(Resolved<T>& resolved, Size row)
		{
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return reinterpret_cast<T*>(resolved.base_)[row];
			}
			else
			{
				return *resolved.ptr_;
			}
		}

	private:
		SC_CONSTEX Bool hasArchetype_ = ((ComponentTraits<StripAccessType<Ts>>::storage == ComponentStorage::Archetype) || ...);

		static inline DynamicArray<Match> matches_;

		static inline std::atomic<Size> checked_ = 0;

		static inline std::mutex mutex_;

		World& world_;

		ArchetypeStorage& archetypes_;

		SparseSetStorage& sparseSets_;
	};
}
