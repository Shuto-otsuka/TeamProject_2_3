#include <GraphicsEngine/D3D12/PipelineState/SamplerState.h>

namespace SeedCore
{
	StaticArray<D3D12_STATIC_SAMPLER_DESC, static_cast<Size>(SamplerStateType::Count)> SamplerState::Create()
	{
		StaticArray<D3D12_TEXTURE_ADDRESS_MODE, 3> addressModes = { D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_MIRROR };
		StaticArray<D3D12_STATIC_SAMPLER_DESC, static_cast<Size>(SamplerStateType::Count)> samplers{};
		
		for (Size index = 0;index < samplers.size();++index)
		{
			SamplerStateType type = static_cast<SamplerStateType>(index);
			Bool border = type == SamplerStateType::BorderBlack || type == SamplerStateType::BorderWhite;
			D3D12_TEXTURE_ADDRESS_MODE addressMode = border ? D3D12_TEXTURE_ADDRESS_MODE_BORDER : addressModes[index % 3];
			D3D12_STATIC_SAMPLER_DESC& sampler = samplers[index];
			sampler.Filter = index <= static_cast<Size>(SamplerStateType::PointMirror) ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			sampler.AddressU = addressMode;
			sampler.AddressV = addressMode;
			sampler.AddressW = addressMode;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = type == SamplerStateType::BorderWhite ? D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE : D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = static_cast<Uint>(index);
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		}
		return samplers;
	}
}