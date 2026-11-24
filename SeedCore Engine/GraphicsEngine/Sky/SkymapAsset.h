#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* On-disk ".skymap" asset: the decoded equirectangular HDR (pixels +
	* dimensions + format), so loading it skips the DirectXTex HDR decode.
	* The GPU IBL convolution still runs on load.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ディスク上の ".skymap" アセット: デコード済みパノラマ HDR（ピクセル +
	* 寸法 + フォーマット）。読み込むときに DirectXTex の HDR デコードを
	* 省略できる。GPU の IBL 畳み込みはロード時に実行する。
	*/
	struct SkymapAsset
	{
		/// [EN] DXGI_FORMAT of the stored pixels.
		/// [JP] 格納しているピクセルの DXGI_FORMAT。
		Uint32 format_ = 0;

		/// [EN] Equirect width in texels.
		/// [JP] equirect の幅（テクセル）。
		Uint32 width_ = 0;

		/// [EN] Equirect height in texels.
		/// [JP] equirect の高さ（テクセル）。
		Uint32 height_ = 0;

		/// [EN] Bytes per row of the stored pixels.
		/// [JP] 格納しているピクセルの1行あたりのバイト数。
		Uint32 rowPitch_ = 0;

		/// [EN] The pixel payload.
		/// [JP] ピクセルの本体。
		DynamicArray<Uint8> pixels_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("format", format_);
			archive.Field("width", width_);
			archive.Field("height", height_);
			archive.Field("rowPitch", rowPitch_);
			archive.Field("pixels", pixels_);
		}
	};
}
