#pragma once

namespace SeedCore
{
	class FontResource;
	class BindlessHeap;
	class D3D12TransferLane;

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
		* recording the uploads into lane, the frame transfer lane the render graph
		* submits before the frame draws, with views in bindlessHeap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* fontResource が持つ全フォントのアトラスを埋め、アップロードを lane（描画の前に
		* レンダーグラフが送信するフレームの転送レーン）に積む。ビューは bindlessHeap に置く。
		*/
		void Update(FontResource& fontResource, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* bindlessHeap);
	};
}
