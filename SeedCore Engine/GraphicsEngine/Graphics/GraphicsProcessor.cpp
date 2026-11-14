#include <GraphicsEngine/Graphics/GraphicsProcessor.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Texture/Compression/BC7CompressShader.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates an empty processor. Nothing touches the GPU until Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 空の加工器を作る。Initialize まで GPU には触らない。
	*/
	GraphicsProcessor::GraphicsProcessor()
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the processor. The pipelines are released by Finalize beforehand.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 加工器を破棄する。パイプラインは、その前に Finalize が解放する。
	*/
	GraphicsProcessor::~GraphicsProcessor() = default;

	/**
	* [EN]
	* Creates the pipeline of every processing kind, compiling its shader
	* through the device's shader cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 加工の種類ごとのパイプラインを、デバイスのシェーダーキャッシュで
	* シェーダーをコンパイルして作る。
	*/
	void GraphicsProcessor::Initialize(RenderDevice& device)
	{
		bc7_ = MakePtr<BC7CompressShader>();
		bc7_->Create(device.ShaderCache(), device.Context().GetDevice());
	}

	/**
	* [EN]
	* Releases every pipeline. Called before the device's shader cache is cleared.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全パイプラインを解放する。デバイスのシェーダーキャッシュを空にする前に呼ぶ。
	*/
	void GraphicsProcessor::Finalize()
	{
		bc7_.reset();
	}

	/**
	* [EN]
	* Returns the BC7 compression shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BC7 圧縮のシェーダーを返す。
	*/
	BC7CompressShader& GraphicsProcessor::BC7()const
	{
		return *bc7_;
	}
}
