#pragma once

namespace SeedCore
{
	class FontResource;
	class BindlessHeap;

	/**
	* [EN]
	* Advances every loaded font once a frame: rasterises the glyphs text
	* asked for since the last frame into the font's MTSDF atlas, then
	* uploads the atlas when it changed. Runs before the renderers so they
	* draw with this frame's glyphs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 読み込み済みの全フォントをフレームに1回進める。前のフレーム以降に
	* テキストが求めたグリフをフォントの MTSDF アトラスへ描き、アトラスが
	* 変わっていればアップロードする。レンダラーが今フレームのグリフで
	* 描けるよう、レンダラーより先に動かす。
	*/
	class FontSystem
	{
	public:
		/**
		* [EN]
		* Fills and uploads the atlas of every font fontResource holds,
		* through cmdQueue into bindlessHeap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* fontResource が持つ全フォントのアトラスを埋め、cmdQueue を通して
		* bindlessHeap へアップロードする。
		*/
		void Update(FontResource& fontResource, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap);
	};
}
