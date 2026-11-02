#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	/**
	* [EN]
	* Releases whatever this manager still holds after a per-asset
	* Unload pass. Defaults to doing nothing, for managers whose Unload
	* already leaves nothing behind.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット単位の Unload パスの後に、このマネージャがまだ保持して
	* いるものを解放する。既定では何もしない（Unload の時点で何も残らない
	* マネージャ向け）。
	*/
	void Asset::Clear()
	{
		/// No Code
	}

	/**
	* [EN]
	* Writes the cache files this manager's asset type derives from the
	* source asset sourceAssetID, appending the full path of every file
	* it wrote to writtenPaths, and returns whether every write
	* succeeded. This is the only place a cache file is written: Load
	* only ever reads one. Defaults to writing nothing, for an asset
	* type that keeps no cache file.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 元アセット sourceAssetID から、このマネージャのアセット種別が作る
	* キャッシュファイルを書き出し、書いた全ファイルのフルパスを
	* writtenPaths へ追加して、全ての書き込みが成功したかどうかを返す。
	* キャッシュファイルを書くのはここだけで、Load は読むだけにする。
	* 既定では何も書かない（キャッシュファイルを持たないアセット種別向け）。
	*/
	Bool Asset::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		return true;
	}

	/**
	* [EN]
	* Writes the in-memory object loaded under assetID to filePath, so an
	* edit made in memory becomes the file's contents, and returns
	* whether it succeeded. Defaults to refusing, for an asset type whose
	* loaded form cannot be written back (it lives on the GPU or in an
	* external library).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているメモリ上のオブジェクトを filePath へ
	* 書き出し、メモリ上での編集をファイルの内容にする。成功したかどうかを
	* 返す。既定では断る（読み込んだ形が GPU 上や外部ライブラリ内にあり、
	* 書き戻せないアセット種別向け）。
	*/
	Bool Asset::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		return false;
	}
}
