#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct AudioLoaderSystem;

	class Sound;
	class ResourceCache;

	/**
	* [EN]
	* Asset resource that maps audio asset IDs to loaded Sound handles and
	* coordinates their lifetime with AudioLoader. Generate converts a
	* source file into its .audio cache, which is the only form a sound
	* plays from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声アセット ID と読み込み済み Sound ハンドルを対応付け、その寿命を
	* AudioLoader と連携して管理するアセットリソース。Generate は素材
	* ファイルを .audio キャッシュへ変換する。音はこの形からしか再生しない。
	*/
	class SEEDCORE_API AudioResource :public AssetResource<Sound>
	{
	public:
		/**
		* [EN]
		* Binds the resource to the loader bundle it draws its audio loader
		* from, starting with no asset loaded. loaders must outlive this
		* resource.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 音声のローダーを引くためのローダーのまとまりに、このリソースを
		* 結び付ける。読み込み済みのアセットが無い状態から始まる。loaders は
		* このリソースより長く生存している必要がある。
		*/
		AudioResource(AudioLoaderSystem& loaders);

		/**
		* [EN]
		* Destroys the audio-resource map.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 音声リソースマップを破棄する。
		*/
		~AudioResource() = default;

		/**
		* [EN]
		* Loads an audio asset through the generic asset context.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 汎用アセットコンテキストを通じて音声アセットを読み込む。
		*/
		void Load(const AssetContext& context, Uint32 assetID)override;

		/**
		* [EN]
		* Loads an audio asset if needed and returns its cached Sound handle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 必要に応じて音声アセットを読み込み、キャッシュ済みの Sound ハンドルを返す。
		*/
		Handle<Sound> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the Sound loaded for an asset ID and removes the mapping.
		* Does nothing when the asset is not loaded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID に対して読み込まれた Sound を解放し、対応を削除する。
		* 読み込まれていなければ何もしない。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Converts the audio source sourceAssetID into the encrypted .audio
		* cache beside it, overwriting any existing one. Nothing is written
		* for an asset that already is a cache, or for a ".awb" streamed
		* alongside its ".acb". Returns false when the source cannot be
		* converted or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 音声素材 sourceAssetID を、その隣の暗号化された .audio キャッシュへ
		* 変換する（既存があれば上書き）。既にキャッシュであるアセットと、
		* ".acb" と一緒にストリーミングされる ".awb" には何も書かない。素材を
		* 変換できない、または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a Sound handle through the audio loader.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* オーディオローダーを通じて Sound ハンドルを解決する。
		*/
		Sound* Resolve(const Handle<Sound>& handle);

	private:
		/// [EN] Loader bundle owned by AudioModule; this resource only uses it.
		/// [JP] AudioModule が所有するローダーのまとまり。このリソースは使うだけ。
		AudioLoaderSystem& loaders_;
	};
}
