#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class Archetype;

	/**
	* [EN]
	* Process-wide, static cache of Archetype instances keyed by a hash
	* of their (sorted) component layout, so that any two identical
	* layouts always resolve to the same Archetype instance rather than
	* being duplicated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンポーネントレイアウト（ソート済み）のハッシュをキーとした、
	* プロセス全体で共有される静的な Archetype インスタンスキャッシュ。
	* 同一のレイアウトが2つあった場合、重複して生成されるのではなく、
	* 常に同じ Archetype インスタンスへ解決される。
	*/
	class SEEDCORE_API ArchetypeRegistry
	{
	private:
		/**
		* [EN]
		* RAII helper whose sole purpose is freeing every registered
		* Archetype at static-destruction time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 静的破棄時に登録済みの全 Archetype を解放することだけを目的とした
		* RAII ヘルパー。
		*/
		struct RegistryCleaner
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
			~RegistryCleaner();
		};

	public:
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
		static Archetype* GetOrCreate(const DynamicArray<ComponentID>& layout);

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
		static std::span<Archetype* const> Archetypes();

	private:
		/// [EN] Every archetype ever created, in creation order; an archetype's position here is its index, and this array owns them.
		/// [JP] これまでに作った全アーキタイプを作った順に持つ。ここでの位置がそのアーキタイプの番号で、この配列が所有する。
		static inline DynamicArray<Archetype*> archetypes_;

		/// [EN] Maps a layout hash to every archetype with that hash; different layouts can share a hash, so each entry is a list.
		/// [JP] レイアウトのハッシュを、そのハッシュを持つ全アーキタイプへ対応付ける。違うレイアウトが同じハッシュになりうるので、各項目は一覧。
		static inline FlatMap<Size, DynamicArray<Archetype*>> registry_;

		/// [EN] Static instance whose destructor frees every archetype at program exit; declared last so it is destroyed while archetypes_ is still alive.
		/// [JP] プログラム終了時にデストラクタが全アーキタイプを解放する静的インスタンス。最後に宣言し、archetypes_ がまだ生きている間に破棄されるようにする。
		static inline RegistryCleaner cleaner_;
	};
}
