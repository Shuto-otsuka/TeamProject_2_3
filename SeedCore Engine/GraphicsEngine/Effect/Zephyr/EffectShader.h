#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>

namespace SeedCore
{
	class ComputeShader;
	class ShaderHotReload;

	struct EffectPipelineState
	{
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> spawnPipelineStateObject_;
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> updatePipelineStateObject_;
	};

	class EffectShader
	{
	public:
		EffectShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~EffectShader() = default;

		void Create(ShaderHotReload& shaderHotReload, ID3D12Device* device);

		[[nodiscard]] EffectPipelineState Create(ShaderHotReload& shaderHotReload, ID3D12Device* device, const DynamicArray<String>& moduleNames);

	public:
		[[nodiscard]] ID3D12PipelineState* GetInitializePipelineStateObject()const;

		[[nodiscard]] ID3D12PipelineState* GetPreparePipelineStateObject()const;

		[[nodiscard]] ID3D12PipelineState* GetGeneratedPipelineStateObject(const Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>>& handle)const;

		[[nodiscard]] ID3D12RootSignature* GetRootSignature()const;

	private:
		Handle<ComputeShader> initializeShader_;
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> initializePipelineStateObject_;

		Handle<ComputeShader> prepareShader_;
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> preparePipelineStateObject_;

		Handle<RootSignature> effectRootSignature_;

		RootSignature& rootSignature_;
		PipelineStateObject& pipelineStateObject_;

		ZephyrModuleAssembler assembler_;
	};
}