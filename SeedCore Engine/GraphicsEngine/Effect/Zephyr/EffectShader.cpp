#include <GraphicsEngine/Effect/Zephyr/EffectShader.h>
#include <FoundationEngine/Log/Error.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>

namespace SeedCore
{
	EffectShader::EffectShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) :rootSignature_(rootSignature), pipelineStateObject_(pipelineStateObject)
	{
		/// No Code
	}

	void EffectShader::Create(ShaderHotReload& shaderHotReload, ID3D12Device* device)
	{
		String path = String("../GraphicsEngine/Effect/Zephyr/ParticleSystemCS.hlsl");

		effectRootSignature_ = rootSignature_.GetOrCreate(device);

		{
			initializeShader_ = shaderHotReload.GetOrCreateComputeShader(path, String("InitializeMain"));

			ComputeShader* computeShader = shaderHotReload.GetComputeShader(initializeShader_);
			if (!computeShader)
			{
				SC_LOG_ERROR("Zephyrシステム: Initialize シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(path));
				return;
			}

			PipelineStateKey psoKey{};
			memset(&psoKey, 0, sizeof(psoKey));
			psoKey.rootSignature_ = rootSignature_.Get(effectRootSignature_)->Get();
			psoKey.computeShader_ = computeShader->Bytecode();
			initializePipelineStateObject_ = pipelineStateObject_.GetOrCreate(device, psoKey);
		}

		{
			prepareShader_ = shaderHotReload.GetOrCreateComputeShader(path, String("PrepareMain"));

			ComputeShader* computeShader = shaderHotReload.GetComputeShader(prepareShader_);
			if (!computeShader)
			{
				SC_LOG_ERROR("Zephyrシステム: Prepare シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(path));
				return;
			}

			PipelineStateKey psoKey{};
			memset(&psoKey, 0, sizeof(psoKey));
			psoKey.rootSignature_ = rootSignature_.Get(effectRootSignature_)->Get();
			psoKey.computeShader_ = computeShader->Bytecode();
			preparePipelineStateObject_ = pipelineStateObject_.GetOrCreate(device, psoKey);
		}
	}

	EffectPipelineState EffectShader::Create(ShaderHotReload& shaderHotReload, ID3D12Device* device, const DynamicArray<String>& moduleNames)
	{
		EffectPipelineState pipelineState{};

		ZephyrGeneratedShader generatedShader = assembler_.GetOrCreate(moduleNames, shaderHotReload);

		{
			ComputeShader* computeShader = shaderHotReload.GetComputeShader(generatedShader.spawnShader_);
			if (!computeShader)
			{
				SC_LOG_ERROR("Zephyrシステム: Spawn シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(generatedShader.generatedFilePath_));
				return pipelineState;
			}

			PipelineStateKey psoKey{};
			memset(&psoKey, 0, sizeof(psoKey));
			psoKey.rootSignature_ = rootSignature_.Get(effectRootSignature_)->Get();
			psoKey.computeShader_ = computeShader->Bytecode();
			pipelineState.spawnPipelineStateObject_ = pipelineStateObject_.GetOrCreate(device, psoKey);
		}

		{
			ComputeShader* computeShader = shaderHotReload.GetComputeShader(generatedShader.updateShader_);
			if (!computeShader)
			{
				SC_LOG_ERROR("Zephyrシステム: Update シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(generatedShader.generatedFilePath_));
				return pipelineState;
			}

			PipelineStateKey psoKey{};
			memset(&psoKey, 0, sizeof(psoKey));
			psoKey.rootSignature_ = rootSignature_.Get(effectRootSignature_)->Get();
			psoKey.computeShader_ = computeShader->Bytecode();
			pipelineState.updatePipelineStateObject_ = pipelineStateObject_.GetOrCreate(device, psoKey);
		}

		return pipelineState;
	}

	ID3D12PipelineState* EffectShader::GetInitializePipelineStateObject()const
	{
		return pipelineStateObject_.Get(initializePipelineStateObject_);
	}

	ID3D12PipelineState* EffectShader::GetPreparePipelineStateObject()const
	{
		return pipelineStateObject_.Get(preparePipelineStateObject_);
	}

	ID3D12PipelineState* EffectShader::GetGeneratedPipelineStateObject(const Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>>& handle)const
	{
		return pipelineStateObject_.Get(handle);
	}

	ID3D12RootSignature* EffectShader::GetRootSignature()const
	{
		return rootSignature_.Get(effectRootSignature_)->Get();
	}
}