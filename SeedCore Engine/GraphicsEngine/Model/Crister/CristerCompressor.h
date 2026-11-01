#pragma once
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>

namespace SeedCore
{
	class BC7CompressShader;
	class D3D12CommandQueue;

	/**
	* [EN]
	* Bakes Bitmaps for the GPU: shrinks images beyond the D3D12 size
	* limit, fills the color of transparent texels, builds the full mip
	* chain, and BC7-compresses every mip on the GPU into each Bitmap's
	* cacheData_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Bitmap を GPU 向けに焼き込む。D3D12 の寸法の上限を超える画像を
	* 縮小し、透明なテクセルの色を埋め、フルミップチェーンを作り、全
	* ミップを GPU で BC7 圧縮して各 Bitmap の cacheData_ へ書き込む。
	*/
	class SEEDCORE_API CristerCompressor
	{
	public:
		/**
		* [EN]
		* Downsamples oversized textures, dilates transparent-texel color
		* (before compression, since BC7 blocks cannot be edited per texel
		* afterwards), then BC7-compresses a full mip chain into each
		* Bitmap's cacheData_. Must run before serialising to the .crister
		* cache. The BC7 encoding runs on the GPU (BC7CompressCS.hlsl, mode
		* 6 only) as a one-shot compute dispatch on cmdQueue, with the root
		* signature and PSO owned by bc7Shader. The shader binds its input
		* and output as root SRV/UAV, so unlike Upload() this needs no
		* BindlessHeap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 大きすぎるテクスチャをダウンサンプルし、透明テクセルの色を
		* dilation する（圧縮後は BC7 ブロックをテクセル単位で編集できない
		* ため、圧縮前に行う）。その後フルミップチェーンを BC7 圧縮して各
		* Bitmap の cacheData_ へ焼き込む。.crister キャッシュへの
		* シリアライズ前に実行すること。BC7 エンコードは GPU で行い
		* (BC7CompressCS.hlsl、mode 6 のみ)、cmdQueue 上の一発実行の
		* コンピュートディスパッチとして、bc7Shader が持つルートシグネチャと
		* PSO で動かす。シェーダーは入出力をルート SRV/UAV でバインドするので、
		* Upload() と違い BindlessHeap は要らない。
		*/
		void BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, std::span<Bitmap> bitmaps)const;
	};
}