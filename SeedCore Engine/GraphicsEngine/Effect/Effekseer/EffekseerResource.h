#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded Effekseer effect
	* handles. Generate writes a ".efkefc" source's ".effekseer" cache,
	* which carries every file the effect reads.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済みの Effekseer エフェクトのハンドル間の
	* マッピングを管理する。Generate は ".efkefc" ソースの ".effekseer"
	* キャッシュを書く。キャッシュはエフェクトが読む全ファイルを持つ。
	*/
	class EffekseerResource :public AssetResource<EffekseerEffect>
	{
	public:
		/**
		* [EN]
		* Takes the loaders.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーを受け取る。
		*/
		EffekseerResource(GraphicsLoaderSystem& loaders);
		~EffekseerResource() = default;

		/**
		* [EN]
		* Loads the asset registered under assetID (the entry point
		* ResourceCache drives).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で登録されているアセットを読み込む（ResourceCache が動かす
		* 入口）。
		*/
		void Load(const AssetContext& context, Uint32 assetID)override;

		/**
		* [EN]
		* Loads assetID unless it is already loaded and returns its handle,
		* or a null handle when the asset is unknown or cannot be created.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID がまだ読み込まれていなければ読み込み、そのハンドルを返す。
		* 未知のアセットか作れない場合は null のハンドルを返す。
		*/
		Handle<EffekseerEffect> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the effect loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているエフェクトを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Writes the ".effekseer" cache beside the ".efkefc" source
		* sourceAssetID. Nothing is written for an asset that is not a
		* ".efkefc". Returns false when the source cannot be read or created,
		* or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ".efkefc" ソース sourceAssetID の隣に ".effekseer" キャッシュを書く。
		* ".efkefc" でないアセットには何も書かない。ソースを読めない、作れない、
		* または書き込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Resolves a handle to an EffekseerEffect pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを EffekseerEffect ポインタに解決する。
		*/
		EffekseerEffect* Resolve(const Handle<EffekseerEffect>& handle);

	private:
		/// [EN] Loaders this manager creates effects and writes ".effekseer" caches through.
		/// [JP] このマネージャがエフェクトを作り、".effekseer" キャッシュを書くのに使うローダー。
		GraphicsLoaderSystem& loaders_;
	};
}
