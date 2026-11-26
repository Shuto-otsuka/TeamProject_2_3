#pragma once

namespace SeedCore
{
	class Archetype;
	class Chunk;

	struct ArchetypeLocation
	{
		Archetype* archetype_ = nullptr;

		Chunk* chunk_ = nullptr;

		Uint32 row_ = UINT32_MAX;
	};
}