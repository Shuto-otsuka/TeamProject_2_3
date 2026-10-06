#include <GraphicsEngine/Shape/Primitive/PrimitiveWireframeShader.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/PipelineState/RasterizerState.h>
#include <GraphicsEngine/D3D12/PipelineState/BlendState.h>

namespace SeedCore
{
	PrimitiveWireframeShader::PrimitiveWireframeShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : rootSignature_(rootSignature), pipelineStateObject_(pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Compiles the shaders and builds both pipeline states: the debug
	* overlay's, depth-tested without depth writes, and the canvas's,
	* without depth.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シェーダーをコンパイルし、2つのパイプラインステートを作る。デバッグの
	* 重ね描き用は深度テストを行い深度は書き込まない。Canvas 用は深度を
	* 使わない。
	*/
	void PrimitiveWireframeShader::Create(ShaderCache& shaderCache, ID3D12Device* device)
	{
		rootSignatureHandle_ = rootSignature_.GetOrCreate(device);
		pixelShader_ = shaderCache.GetOrCreatePixelShader(String("../GraphicsEngine/Shape/Primitive/PrimitiveWireframePS.hlsl"));

		PipelineStateKey psoKey{};
		memset(&psoKey, 0, sizeof(psoKey));
		psoKey.rootSignature_ = rootSignature_.Get(rootSignatureHandle_)->Get();

		/// [EN] Each line is expanded into a quad on the GPU: by the mesh shader on D12_2, by the vertex shader below it.
		/// [JP] 各線は GPU 上で四角形に展開する。D12_2 ではメッシュシェーダ、それ未満では頂点シェーダが行う。
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			meshShader_ = shaderCache.GetOrCreateMeshShader(String("../GraphicsEngine/Shape/Primitive/PrimitiveWireframeMS.hlsl"));
			psoKey.meshShader_ = shaderCache.GetMeshShader(meshShader_)->Bytecode();
			psoKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}
		else
		{
			vertexShader_ = shaderCache.GetOrCreateVertexShader(String("../GraphicsEngine/Shape/Primitive/PrimitiveWireframeVS.hlsl"));
			psoKey.vertexShader_ = shaderCache.GetVertexShader(vertexShader_)->Bytecode();
			psoKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}

		psoKey.pixelShader_ = shaderCache.GetPixelShader(pixelShader_)->Bytecode();
		psoKey.rasterizerDesc_ = RasterizerState::Get(RasterizerStateType::SolidNoneLHS);
		psoKey.blendDesc_ = BlendState::Get(BlendStateType::Opaque);

		/// [EN] Same target as the collider lines' debug overlay: the post-tonemap 8-bit output with the resized depth.
		/// [JP] コライダーの線のデバッグの重ね描きと同じ描画先。トーンマップ後の 8 ビット出力と、大きさを合わせた深度。
		psoKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOnWriteOffReverseZ);
		psoKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoKey.renderTargetViewCount_ = 1;
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_D32_FLOAT;
		pipelineState_ = pipelineStateObject_.GetOrCreate(device, psoKey);

		/// [EN] Same target as the collider lines on the canvas: the canvas frame buffer, drawn over everything without depth.
		/// [JP] Canvas 上のコライダーの線と同じ描画先。Canvas のフレームバッファへ、深度を使わずに上から描く。
		psoKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOff);
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_UNKNOWN;
		pipelineStateCanvas_ = pipelineStateObject_.GetOrCreate(device, psoKey);
	}

	/**
	* [EN]
	* Returns the pipeline state for the editor view's debug overlay.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタービューのデバッグの重ね描き用のパイプラインステートを返す。
	*/
	ID3D12PipelineState* PrimitiveWireframeShader::GetPipelineState()const
	{
		return pipelineStateObject_.Get(pipelineState_);
	}

	/**
	* [EN]
	* Returns the pipeline state for the canvas view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Canvas ビュー用のパイプラインステートを返す。
	*/
	ID3D12PipelineState* PrimitiveWireframeShader::GetPipelineStateCanvas()const
	{
		return pipelineStateObject_.Get(pipelineStateCanvas_);
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
	ID3D12RootSignature* PrimitiveWireframeShader::GetRootSignature()const
	{
		return rootSignature_.Get(rootSignatureHandle_)->Get();
	}
}
