#pragma once

namespace SeedCore
{
	enum class BlendStateType
	{
		Opaque,
		Alpha,

		AlphaPremultiplied,

		Additive,
		Multiply,
		Subtractive,
		Screen,
	};

	class BlendState
	{
	public:
		static void Create();

		static D3D12_BLEND_DESC Get(BlendStateType type);
	};
}
