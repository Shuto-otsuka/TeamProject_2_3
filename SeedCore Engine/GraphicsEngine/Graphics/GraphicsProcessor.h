#pragma once

namespace SeedCore
{
	class BC7CompressShader;
	class RenderDevice;

	/**
	* [EN]
	* GPU processing of asset data that runs outside the render graph, once
	* per asset while it is loaded or baked: block compression (BC7) now,
	* and further formats and mip generation alongside it. Graphics owns one
	* and hands it to GraphicsModule and the bakers that process textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レンダーグラフの外で、アセットを読み込むときや焼き込むときに 1 回ずつ
	* 走る、アセットのデータの GPU での加工。今はブロック圧縮（BC7）で、
	* ほかの形式やミップの生成もここに並べる。Graphics が 1 つ持ち、
	* GraphicsModule と、テクスチャを加工するベイカーに渡す。
	*/
	class SEEDCORE_API GraphicsProcessor
	{
	public:
		/**
		* [EN]
		* Creates an empty processor. Nothing touches the GPU until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空の加工器を作る。Initialize まで GPU には触らない。
		*/
		GraphicsProcessor();

		/**
		* [EN]
		* Destroys the processor. The pipelines are released by Finalize beforehand.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 加工器を破棄する。パイプラインは、その前に Finalize が解放する。
		*/
		~GraphicsProcessor();

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
		void Initialize(RenderDevice& device);

		/**
		* [EN]
		* Releases every pipeline. Called before the device's shader cache is cleared.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全パイプラインを解放する。デバイスのシェーダーキャッシュを空にする前に呼ぶ。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Returns the BC7 compression shader. The reference stays valid until Finalize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* BC7 圧縮のシェーダーを返す。参照は Finalize まで有効である。
		*/
		NODISCARD BC7CompressShader& BC7()const;

	private:
		/// [EN] Root signature and pipeline of the BC7 compression.
		/// [JP] BC7 圧縮のルートシグネチャとパイプライン。
		ResourcePtr<BC7CompressShader> bc7_;
	};
}
