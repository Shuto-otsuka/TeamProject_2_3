#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct AudioLoaderSystem;

	class Sound;
	class ResourceCache;

	/**
	* [EN]
	* Asset resource that maps audio asset IDs to loaded Sound handles and
	* coordinates their lifetime with AudioLoader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声アセット ID と読み込み済み Sound ハンドルを対応付け、その寿命を
	* AudioLoader と連携して管理するアセットリソース。
	*/
	class SEEDCORE_API AudioResource :public Asset, public NonCopyable
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
		Handle<Sound> Load(ResourceCache& cache, Uint32 assetID);

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
		* Returns the loaded Sound handle associated with an asset ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID に対応する読み込み済み Sound ハンドルを返す。
		*/
		Handle<Sound> GetHandle(Uint32 assetID)const;

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

		/**
		* [EN]
		* Reports whether an asset ID has a loaded Sound handle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID に読み込み済み Sound ハンドルがあるかを返す。
		*/
		Bool Contains(Uint32 assetID)const;

	private:
		/// [EN] Loader bundle owned by AudioModule; this resource only uses it.
		/// [JP] AudioModule が所有するローダーのまとまり。このリソースは使うだけ。
		AudioLoaderSystem& loaders_;

		/// [EN] Loaded Sound handle indexed by audio asset ID.
		/// [JP] 音声アセット ID をキーとする読み込み済み Sound ハンドル。
		FlatMap<Uint32, Handle<Sound>> assetHandleMap_;
	};
}
