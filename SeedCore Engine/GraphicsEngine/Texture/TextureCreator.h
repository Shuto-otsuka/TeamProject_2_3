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
		* writes its shader resource view at textureIndex of heap, recording
		* the upload into resourceUpload. A sibling ".texture" cache newer
		* than the source is read instead of the source; a ".icon", ".logo"
		* or ".texture" file is read as such a cache directly. Nothing is
		* written: the cache is made by TextureResource::Generate.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の画像ファイルから resource にテクスチャを作り、heap の
		* textureIndex にシェーダーリソースビューを書く。アップロードは
		* resourceUpload に記録する。ソースより新しい隣の ".texture"
		* キャッシュがあれば、ソースの代わりにそれを読む。".icon"・".logo"・
		* ".texture" のファイルは、そのままキャッシュとして読む。何も書かない
		* （キャッシュは TextureResource::Generate が作る）。
		*/
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Same as the ResourceUploadBatch overload, but submits the upload to
		* cmdQueue and waits for it before returning.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResourceUploadBatch 版と同じだが、アップロードを cmdQueue へ投入し、
		* 完了を待ってから戻る。
		*/
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Creates a texture from DDS or WIC image bytes in data into resource
		* and writes its shader resource view at textureIndex of heap,
		* recording the upload into resourceUpload. resource is reset when
		* the bytes cannot be decoded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* data の DDS か WIC の画像バイト列から resource にテクスチャを作り、
		* heap の textureIndex にシェーダーリソースビューを書く。アップロードは
		* resourceUpload に記録する。デコードできなければ resource を空にする。
		*/
		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		/**
		* [EN]
		* Same as the ResourceUploadBatch overload, but submits the upload to
		* cmdQueue and waits for it before returning.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResourceUploadBatch 版と同じだが、アップロードを cmdQueue へ投入し、
		* 完了を待ってから戻る。
		*/
		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

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
