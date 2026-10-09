#include <GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidShader.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/D3D12/PipelineState/AmplificationShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>
#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/PipelineState/RasterizerState.h>
#include <GraphicsEngine/D3D12/PipelineState/BlendState.h>
#include <GraphicsEngine/D3D12/PipelineState/DepthStencilState.h>

namespace SeedCore
{
	PrimitiveSolidShader::PrimitiveSolidShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : rootSignature_(rootSignature), pipelineStateObject_(pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Compiles the shaders for the current feature level and builds its
	* pipeline states: the amplification and mesh shader one on D12_2,
	* or the culling compute one and the single-sided and double-sided
	* vertex shader ones below it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 今の機能レベル用のシェーダーをコンパイルし、パイプラインステートを
	* 作る。D12_2 では Amplification Shader とメッシュシェーダーのもの、
	* それ未満ではカリングの Compute Shader のものと、片面用・両面用の
	* 頂点シェーダーのものを作る。
	*/
	void PrimitiveSolidShader::Create(ShaderCache& shaderCache, ID3D12Device* device)
	{
		rootSignatureHandle_ = rootSignature_.GetOrCreate(device);
		pixelShader_ = shaderCache.GetOrCreatePixelShader(String("../GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidPS.hlsl"));

		/// [EN] Same target as the wireframe's debug overlay: the post-tonemap 8-bit output with the resized depth, here written as well as tested so the surfaces hide each other.
		/// [JP] ワイヤーフレームのデバッグの重ね描きと同じ描画先。トーンマップ後の 8 ビット出力と、大きさを合わせた深度。面どうしが隠し合うよう、ここでは深度をテストするだけでなく書き込む。
		PipelineStateKey psoKey{};
		memset(&psoKey, 0, sizeof(psoKey));
		psoKey.rootSignature_ = rootSignature_.Get(rootSignatureHandle_)->Get();
		psoKey.pixelShader_ = shaderCache.GetPixelShader(pixelShader_)->Bytecode();
		psoKey.blendDesc_ = BlendState::Get(BlendStateType::Opaque);
		psoKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOnWriteOnReverseZ);
		psoKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoKey.renderTargetViewCount_ = 1;
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_D32_FLOAT;

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			/// [EN] Fixed-function culling stays off; the mesh shader drops the back faces of single-sided shapes itself, so double-sided ones share this pipeline.
			/// [JP] 固定機能のカリングは使わない。片面の形の裏面はメッシュシェーダーが自分で捨てるので、両面の形も同じパイプラインで描ける。
			amplificationShader_ = shaderCache.GetOrCreateAmplificationShader(String("../GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidAS.hlsl"));
			meshShader_ = shaderCache.GetOrCreateMeshShader(String("../GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidMS.hlsl"));
			psoKey.amplificationShader_ = shaderCache.GetAmplificationShader(amplificationShader_)->Bytecode();
			psoKey.meshShader_ = shaderCache.GetMeshShader(meshShader_)->Bytecode();
			psoKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			psoKey.rasterizerDesc_ = RasterizerState::Get(RasterizerStateType::SolidNoneLHS);
			pipelineState_ = pipelineStateObject_.GetOrCreate(device, psoKey);
		}
		else
		{
			/// [EN] The single-sided list culls back faces in the rasterizer; the double-sided list keeps both.
			/// [JP] 片面用の一覧はラスタライザで裏面を捨て、両面用の一覧は両面を残す。
			vertexShader_ = shaderCache.GetOrCreateVertexShader(String("../GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidVS.hlsl"));
			psoKey.vertexShader_ = shaderCache.GetVertexShader(vertexShader_)->Bytecode();
			psoKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoKey.rasterizerDesc_ = RasterizerState::Get(RasterizerStateType::SolidBackLHS);
			pipelineStateSingleSided_ = pipelineStateObject_.GetOrCreate(device, psoKey);
			psoKey.rasterizerDesc_ = RasterizerState::Get(RasterizerStateType::SolidNoneLHS);
			pipelineStateDoubleSided_ = pipelineStateObject_.GetOrCreate(device, psoKey);

			computeShader_ = shaderCache.GetOrCreateComputeShader(String("../GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidCullingCS.hlsl"));
			PipelineStateKey cullingKey{};
			memset(&cullingKey, 0, sizeof(cullingKey));
			cullingKey.rootSignature_ = rootSignature_.Get(rootSignatureHandle_)->Get();
			cullingKey.computeShader_ = shaderCache.GetComputeShader(computeShader_)->Bytecode();
			pipelineStateCulling_ = pipelineStateObject_.GetOrCreate(device, cullingKey);
		}
	}

	/**
	* [EN]
	* Returns the amplification and mesh shader pipeline state (D12_2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Amplification Shader とメッシュシェーダーのパイプラインステートを
	* 返す（D12_2）。
	*/
	ID3D12PipelineState* PrimitiveSolidShader::GetPipelineState()const
	{
		return pipelineStateObject_.Get(pipelineState_);
	}

	/**
	* [EN]
	* Returns the culling compute pipeline state (below D12_2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カリングの Compute Shader のパイプラインステートを返す（D12_2 未満）。
	*/
	ID3D12PipelineState* PrimitiveSolidShader::GetPipelineStateCulling()const
	{
		return pipelineStateObject_.Get(pipelineStateCulling_);
	}

	/**
	* [EN]
	* Returns the vertex shader pipeline state that culls back faces, for
	* the single-sided list (below D12_2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 裏面を捨てる頂点シェーダーのパイプラインステートを返す。片面用の
	* 一覧に使う（D12_2 未満）。
	*/
	ID3D12PipelineState* PrimitiveSolidShader::GetPipelineStateSingleSided()const
	{
		return pipelineStateObject_.Get(pipelineStateSingleSided_);
	}

	/**
	* [EN]
	* Returns the vertex shader pipeline state that keeps both faces, for
	* the double-sided list (below D12_2).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 両面を残す頂点シェーダーのパイプラインステートを返す。両面用の
	* 一覧に使う（D12_2 未満）。
	*/
	ID3D12PipelineState* PrimitiveSolidShader::GetPipelineStateDoubleSided()const
	{
		return pipelineStateObject_.Get(pipelineStateDoubleSided_);
	}

	/**
	* [EN]
	* Returns the root signature.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ルートシグネチャを返す。
	*/
	ID3D12RootSignature* PrimitiveSolidShader::GetRootSignature()const
	{
		return rootSignature_.Get(rootSignatureHandle_)->Get();
	}
}
