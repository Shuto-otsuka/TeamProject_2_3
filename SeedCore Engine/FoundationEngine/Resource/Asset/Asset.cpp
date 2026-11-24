#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads the ".meta" beside the asset file at filePath into this
	* object and returns true. Returns false and leaves every field as it
	* was when that ".meta" does not exist or cannot be read, so a caller
	* sets the values it wants as defaults before reading.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath のアセットファイルの隣にある ".meta" を、このオブジェクトへ
	* 読み込んで true を返す。その ".meta" が無い、または読めない場合は
	* false を返し、全てのフィールドを元のまま残す。なので呼び出し側は、
	* 既定にしたい値を読む前に入れておく。
	*/
	Bool AssetMeta::Read(const std::filesystem::path& filePath)
	{
		std::filesystem::path metaPath = filePath;
		metaPath += ".meta";
		if (!std::filesystem::exists(metaPath))
		{
			return false;
		}

		/// [EN] The archive reads the whole file before anything is deserialized, so a file it cannot read leaves the fields untouched.
		/// [JP] アーカイブはデシリアライズの前にファイル全体を読むので、読めないファイルはフィールドに触れない。
		BinaryInputArchive archive;
		if (!archive.Read(String(metaPath.string())))
		{
			return false;
		}
		Serialize(archive);
		return true;
	}

	/**
	* [EN]
	* Writes this object to the ".meta" beside the asset file at
	* filePath, replacing any existing one. Returns false when the write
	* failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このオブジェクトを、filePath のアセットファイルの隣にある ".meta" へ
	* 書き出す。既存のものは置き換える。書き込みに失敗した場合は false を
	* 返す。
	*/
	Bool AssetMeta::Write(const std::filesystem::path& filePath)const
	{
		std::filesystem::path metaPath = filePath;
		metaPath += ".meta";

		/// [EN] Serialize is shared with reading and so is not const; writing does not change any field.
		/// [JP] Serialize は読み込みと共通なので const ではない。書き出しはどのフィールドも変えない。
		BinaryOutputArchive archive;
		const_cast<AssetMeta*>(this)->Serialize(archive);
		return archive.Write(String(metaPath.string()));
	}

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
