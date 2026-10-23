#pragma once

namespace SeedCore
{
	class Archetype;
	class Chunk;

	/**
	* [EN]
	* Where one entity's archetype-stored components are: the archetype,
	* the chunk within it, and the row within that chunk.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのエンティティのアーキタイプ格納コンポーネントがある場所。
	* アーキタイプ、その中のチャンク、そのチャンク内の行。
	*/
	struct ArchetypeLocation
	{
		/// [EN] Archetype the components are stored in; nullptr while unplaced.
		/// [JP] コンポーネントが格納されているアーキタイプ。未配置の間は nullptr。
		Archetype* archetype_ = nullptr;

		/// [EN] Chunk within archetype_; nullptr while unplaced.
		/// [JP] archetype_ 内のチャンク。未配置の間は nullptr。
		Chunk* chunk_ = nullptr;

		/// [EN] Row within chunk_; UINT32_MAX while unplaced.
		/// [JP] chunk_ 内の行。未配置の間は UINT32_MAX。
		Uint32 row_ = UINT32_MAX;
	};
}