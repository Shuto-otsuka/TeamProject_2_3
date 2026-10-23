#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>

namespace SeedCore
{
	/**
	* [EN]
	* Deletes every Archetype held in archetypes_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* archetypes_ に保持されている全ての Archetype を削除する。
	*/
	ArchetypeRegistry::RegistryCleaner::~RegistryCleaner()
	{
		/// [EN] Archetypes are allocated with new in GetOrCreate and owned by archetypes_ for the whole process.
		/// [JP] アーキタイプは GetOrCreate で new され、プロセスの終わりまで archetypes_ が所有する。
		for (Archetype* archetype : archetypes_)
		{
			delete archetype;
		}
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the existing Archetype matching layout (order-independent),
	* or creates and registers a new one if none exists yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* layout に一致する（順序に依存しない）既存の Archetype を返す。
	* まだ存在しなければ新しく生成して登録する。
	*/
	Archetype* ArchetypeRegistry::GetOrCreate(const DynamicArray<ComponentID>& layout)
	{
		/// [EN] Sort so the same set of components always maps to one archetype, whatever order it was given in.
		/// [JP] どの順で渡されても同じコンポーネント集合が1つのアーキタイプになるよう、並べ替える。
		DynamicArray<ComponentID> sortedLayout = layout;
		std::ranges::sort(sortedLayout);

		/// [EN] Combine every component ID into a single order-independent hash (sortedLayout is sorted, so the same set of components always yields the same hash).
		/// [JP] 全コンポーネント ID を単一の順序非依存なハッシュへ結合する（sortedLayout はソート済みのため、同じコンポーネント集合は常に同じハッシュになる）。
		Size hash = 0;
		for (ComponentID id : sortedLayout)
		{
			/// [EN] boost-style hash_combine: 0x9e3779b9 (golden-ratio constant) and the shifts spread each ID's bits across the whole hash.
			/// [JP] boost の hash_combine と同じ式。0x9e3779b9(黄金比由来の定数)とシフトで、各 ID のビットをハッシュ全体に散らす。
			hash ^= reinterpret_cast<uintptr_t>(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}

		/// [EN] Every archetype sharing the hash is compared by its actual layout, since different sets can share a hash.
		/// [JP] 異なる集合が同じハッシュになりうるので、同じハッシュの全アーキタイプを実際のレイアウトで比べる。
		DynamicArray<Archetype*>& bucket = registry_[hash];
		for (Archetype* existing : bucket)
		{
			/// [EN] The stored layout is sorted too, so a plain element-wise comparison decides it.
			/// [JP] 登録済みのレイアウトもソート済みなので、要素ごとの単純な比較で判定できる。
			if (existing->Layout() == sortedLayout)
			{
				return existing;
			}
		}

		/// [EN] No match yet: the new archetype takes the next index and joins the hash's list.
		/// [JP] まだ無いので、新しいアーキタイプに次の番号を振り、ハッシュの一覧に加える。
		Archetype* newArchetype = new Archetype(sortedLayout, static_cast<Uint32>(archetypes_.size()));
		archetypes_.push_back(newArchetype);
		bucket.push_back(newArchetype);
		return newArchetype;
	}

	/**
	* [EN]
	* Returns every archetype created so far, in creation order; an
	* archetype's position here is its Index().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* これまでに作った全アーキタイプを作った順に返す。ここでの位置が
	* そのアーキタイプの Index() になる。
	*/
	std::span<Archetype* const> ArchetypeRegistry::Archetypes()
	{
		return std::span<Archetype* const>(archetypes_.data(), archetypes_.size());
	}
}
