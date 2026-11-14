#pragma once
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	struct Bitmap;

	class BindlessHeap;
	class D3D12CommandQueue;
	class D3D12TransferLane;

	/**
	* [EN]
	* Builds GPU textures and their shader resource views from a file, from
	* image bytes in memory or from a baked BC7 bitmap. It owns nothing:
	* the loader, the streaming, splash/boot screens and editor panels all
	* create through it, and each keeps what it created.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ファイル、メモリ上の画像バイト列、焼き込み済みの BC7 ビットマップから、
	* GPU テクスチャとそのシェーダーリソースビューを作る。何も所有しない:
	* ローダー、ストリーミング、スプラッシュ/起動画面、エディタのパネルが
	* これを通して作り、作ったものはそれぞれが持つ。
	*/
	class SEEDCORE_API TextureCreator
	{
	public:
		/**
		* [EN]
		* Creates a texture from the image file at filePath into resource and
		* writes its shader resource view at textureIndex of heap, recording the
		* upload into lane, which the caller has begun and submits. A sibling
		* ".texture" cache newer than the source is read instead of the source; a
		* ".icon", ".logo" or ".texture" file is read as such a cache directly.
		* Nothing is written: the cache is made by TextureResource::Generate.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の画像ファイルから resource にテクスチャを作り、heap の
		* textureIndex にシェーダーリソースビューを書く。アップロードは lane に
		* 積む（Begin と Submit は呼び出し側が行う）。ソースより新しい隣の
		* ".texture" キャッシュがあれば、ソースの代わりにそれを読む。".icon"・
		* ".logo"・".texture" のファイルは、そのままキャッシュとして読む。何も
		* 書かない（キャッシュは TextureResource::Generate が作る）。
		*/
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Same as the lane overload, but begins transferLane on copyQueue, submits
		* it and waits for the upload before returning.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* lane 版と同じだが、copyQueue 上で transferLane を Begin し、Submit して
		* アップロードの完了を待ってから戻る。
		*/
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Creates a texture from DDS or WIC image bytes in data into resource and
		* writes its shader resource view at textureIndex of heap, recording the
		* upload of every subresource into lane, which the caller has begun and
		* submits. The bytes are decoded with DirectXTex (DDS first, then WIC);
		* the texture is created in the COMMON state, so the copy queue writes it
		* and the graphics queue reads it without explicit transitions. resource
		* is reset when the bytes cannot be decoded or the texture cannot be
		* created.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data の DDS か WIC の画像バイト列から resource にテクスチャを作り、heap の
		* textureIndex にシェーダーリソースビューを書く。全サブリソースの
		* アップロードは lane に積む（Begin と Submit は呼び出し側が行う）。
		* デコードは DirectXTex で行う（DDS、次に WIC）。テクスチャは COMMON 状態で
		* 作るので、コピーキューが書き、描画のキューが読むのに明示的な遷移が
		* 要らない。デコードできないか作れなければ resource を空にする。
		*/
		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Same as the lane overload, but begins transferLane on copyQueue, submits
		* it and waits for the upload before returning.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* lane 版と同じだが、copyQueue 上で transferLane を Begin し、Submit して
		* アップロードの完了を待ってから戻る。
		*/
		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Builds a texture from a baked BC7 bitmap: uploads every mip through
		* transferLane on copyQueue, waits for it, and writes a shader
		* resource view into a newly allocated index of heap. The texture is
		* pinned, since it has no file streaming could reload it from. The
		* returned texture has no resource when the bitmap is malformed or the
		* GPU resource could not be created; it is handed to the loader with
		* TextureLoader::Adopt.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 焼き込み済みの BC7 ビットマップからテクスチャを組み立てる。全ミップを
		* copyQueue 上の transferLane でアップロードして完了を待ち、heap に
		* 新しく確保した番号へシェーダーリソースビューを書く。読み直す元の
		* ファイルが無いため、ストリーミングで追い出されないよう固定する。
		* ビットマップが不正か GPU リソースを作れなかった場合、返すテクスチャは
		* リソースを持たない。作ったテクスチャは TextureLoader::Adopt で
		* ローダーへ渡す。
		*/
		static Texture CreateTextureBitmap(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN BindlessHeap* heap, SC_IN const Bitmap& bitmap);
	};
}
