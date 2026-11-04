#pragma once

namespace SeedCore
{
	enum class RenderViewKind :Uint8
	{
		Editor,
		Game,
		Canvas,
	};

	inline constexpr Uint32 RenderViewCount = 3;

	enum class RenderViewMask :Uint8
	{
		Frame = 0,
		Editor = 1 << 0,
		Game = 1 << 1,
		Canvas = 1 << 2,
		Scene = Editor | Game,
	};
}