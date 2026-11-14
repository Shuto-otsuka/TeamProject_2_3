#pragma once

namespace SeedCore
{
	enum class SamplerStateType
	{
		PointClamp = 0, 
		PointWrap,
		PointMirror,
		LinearClamp,
		LinearWrap,
		LinearMirror,
		BorderBlack,
		BorderWhite,

		Count,
	};

	class SamplerState
	{
	public:
		static StaticArray<D3D12_STATIC_SAMPLER_DESC, static_cast<Size>(SamplerStateType::Count)> Create();
	};
}