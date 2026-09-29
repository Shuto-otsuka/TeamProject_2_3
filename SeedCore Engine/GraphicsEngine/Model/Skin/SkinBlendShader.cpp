#include <GraphicsEngine/Model/Skin/SkinBlendShader.h>

#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>
#include <GraphicsEngine/Shader/ShaderCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Keeps the shared root signature and pipeline cache the pipeline is
	* created from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パイプラインの生成元になる、共有のルートシグネチャとパイプラインの
	* キャッシュを保持する。
	*/
	SkinBlendShader::SkinBlendShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : rootSignature_(rootSignature), pipelineStateObject_(pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Compiles SkinBlendCS.hlsl and creates its pipeline on the shared root
	* signature.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SkinBlendCS.hlsl をコンパイルし、共有のルートシグネチャ上に
	* パイプラインを生成する。
	*/
	void SkinBlendShader::Create(ShaderCache& shaderCache, ID3D12Device* device)
	{
		rootSignatureHandle_ = rootSignature_.GetOrCreate(device);
		computeShader_ = shaderCache.GetOrCreateComputeShader(String("../GraphicsEngine/Model/Skin/SkinBlendCS.hlsl"));

		PipelineStateKey pipelineStateKey{};
		memset(&pipelineStateKey, 0, sizeof(pipelineStateKey));
		pipelineStateKey.rootSignature_ = GetRootSignature();
		pipelineStateKey.computeShader_ = shaderCache.GetComputeShader(computeShader_)->Bytecode();
		pipelineStateObjectHandle_ = pipelineStateObject_.GetOrCreate(device, pipelineStateKey);
	}

	/**
	* [EN]
	* The shared root signature the pipeline was created on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パイプラインを生成した共有のルートシグネチャ。
	*/
	ID3D12RootSignature* SkinBlendShader::GetRootSignature()const
	{
		return rootSignature_.Get(rootSignatureHandle_)->Get();
	}

	/**
	* [EN]
	* The pipeline of SkinBlendCS.hlsl.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SkinBlendCS.hlsl のパイプライン。
	*/
	ID3D12PipelineState* SkinBlendShader::GetPipelineState()const
	{
		return pipelineStateObject_.Get(pipelineStateObjectHandle_);
	}
}
