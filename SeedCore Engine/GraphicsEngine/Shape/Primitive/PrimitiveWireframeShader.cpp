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
	* Compiles the shaders and builds the three pipeline states: the debug
	* overlay's, depth-tested without depth writes; the canvas's, without
	* depth; and the preview's, onto a 16-bit float target with a depth
	* buffer that is not tested, so the lines sit over the previewed model.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シェーダーをコンパイルし、3つのパイプラインステートを作る。デバッグの
	* 重ね描き用は深度テストを行い深度は書き込まない。Canvas 用は深度を
	* 使わない。プレビュー用は 16 ビット浮動小数の描画先へ、深度バッファは
	* あるがテストせずに描き、線がプレビューのモデルの上に乗るようにする。
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

		/// [EN] The debug overlay draws into the post-tonemap 8-bit output with the resized depth.
		/// [JP] デバッグの重ね描きは、トーンマップ後の 8 ビット出力と、大きさを合わせた深度へ描く。
		psoKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOnWriteOffReverseZ);
		psoKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoKey.renderTargetViewCount_ = 1;
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_D32_FLOAT;
		pipelineState_ = pipelineStateObject_.GetOrCreate(device, psoKey);

		/// [EN] The canvas draws into the canvas frame buffer, over everything without depth.
		/// [JP] Canvas は、Canvas のフレームバッファへ、深度を使わずに上から描く。
		psoKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOff);
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_UNKNOWN;
		pipelineStateCanvas_ = pipelineStateObject_.GetOrCreate(device, psoKey);

		/// [EN] The preview views draw into their own 16-bit float frame buffer, which has a depth buffer bound; it is not tested, so the lines sit over the model.
		/// [JP] プレビューのビューは、自分の 16 ビット浮動小数のフレームバッファへ描く。深度バッファは付いているがテストしないので、線はモデルの上に乗る。
		psoKey.depthStencilViewFormat_ = DXGI_FORMAT_D32_FLOAT;
		pipelineStatePreview_ = pipelineStateObject_.GetOrCreate(device, psoKey);
	}

	/**
	* [EN]
	* Returns the pipeline state for the editor and game views' debug overlay.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディターとゲームのビューのデバッグの重ね描き用のパイプライン
	* ステートを返す。
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
	* Returns the pipeline state for the preview views, drawn over
	* everything.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべての上に描く、プレビューのビュー用のパイプラインステートを返す。
	*/
	ID3D12PipelineState* PrimitiveWireframeShader::GetPipelineStatePreview()const
	{
		return pipelineStateObject_.Get(pipelineStatePreview_);
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
