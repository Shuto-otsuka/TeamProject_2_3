#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/Effect/Zephyr/EffectShader.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrParticleStorage.h>

namespace SeedCore
{
	struct RootAddresses;

	class BindlessHeap;
	class D3D12CommandList;
	class ShaderHotReload;

	class EffectRenderer
	{
	public:
		EffectRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

		~EffectRenderer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderHotReload& shaderHotReload);

		void Prepare(ID3D12Device* device, Float deltaTime, Bool enabled);

		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		EffectShader effectShader_;

		ResourcePtr<ZephyrParticleStorage> storage_;

		EffectPipelineState effectPipelineState_;

		Float spawnAccumulator_ = 0.0f;

		Float emitterAge_ = 0.0f;

		Bool initialized_ = false;

		Bool enabled_ = false;

		Bool pipelineStateMissingLogged_ = false;
	};
}