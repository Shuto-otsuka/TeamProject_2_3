#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/PipelineDescriptionAccess.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	template<class Description>
	class ComputePipeline
	{
	private:
		SC_STATIC_ASSERT(PipelineDescriptionAccess::IsComputeDescription<Description>, "Compute の記述は file_ / entryPoint_ / threadGroupSizeX_ / threadGroupSizeY_ を持つこと");

	public:
		void Create(RenderDevice& device)
		{
			computeShader_ = device.ShaderCache().GetOrCreateComputeShader(Description::file_, String(Description::entryPoint_));
			ComputeShader* computeShader = device.ShaderCache().GetComputeShader(computeShader_);
			if (!computeShader)
			{
				SC_LOG_ERROR("ComputePipeline: シェーダーのコンパイルに失敗しました");
				return;
			}
			PipelineStateKey key{};
			key.rootSignature_ = device.RootSignature().Signature();
			key.computeShader_ = computeShader->Bytecode();
			pipelineState_ = device.PipelineState().GetOrCreate(device.Context().GetDevice(), key);

#if SC_DEBUG
			reflection_ = computeShader->ReflectionBlob();
#endif
		}

		void Dispatch(RenderPass& pass, Uint32 width, Uint32 height, Uint32 depth = 1, Uint32 constant = 0)
		{
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			if (!pipelineState)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(std::span<IDxcBlob* const>(&reflection_, 1));
#endif
			Uint32 groupSizeZ = 1;
			if constexpr (requires{ Description::threadGroupSizeZ_; })
			{
				groupSizeZ = Description::threadGroupSizeZ_;
			}
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState(pipelineState);
			cmdList->SetComputeRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);
			cmdList->Dispatch((width + Description::threadGroupSizeX_ - 1) / Description::threadGroupSizeX_, (height + Description::threadGroupSizeY_ - 1) / Description::threadGroupSizeY_, (depth + groupSizeZ - 1) / groupSizeZ);
		}

	private:
		Handle<ComputeShader> computeShader_;

		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineState_;

#if SC_DEBUG
		IDxcBlob* reflection_ = nullptr;
#endif
	};
}