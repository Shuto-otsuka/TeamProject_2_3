#include <GraphicsEngine/Model/Material/MaterialLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads the ".material" file at filePath into a new BakedSurface and
	* returns its handle, or a null handle when the file is missing, has
	* another extension or cannot be read. thread is unused: a material
	* file needs no per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の ".material" ファイルを新しい BakedSurface へ読み込み、
	* そのハンドルを返す。ファイルが無い、拡張子が違う、読めない場合は
	* null のハンドルを返す。thread は使わない（マテリアルのファイルは
	* スレッドごとの状態を持たないため）。
	*/
	Handle<BakedSurface> MaterialLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".material" || !std::filesystem::exists(path))
		{
			return Handle<BakedSurface>::null();
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			return Handle<BakedSurface>::null();
		}

		Handle<BakedSurface> handle = pool_.Create();
		BakedSurface* bakedSurface = pool_.Get(handle);
		if (!bakedSurface)
		{
			return Handle<BakedSurface>::null();
		}
		bakedSurface->Serialize(archive);

		return handle;
	}

	/**
	* [EN]
	* Destroys the material handle names.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle が示すマテリアルを破棄する。
	*/
	void MaterialLoader::Clear(Handle<BakedSurface>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Serializes bakedSurface to filePath as a ".material" file
	* (overwriting any existing one). Returns false when the write
	* failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bakedSurface を ".material" ファイルとして filePath へ直列化する
	* （既存があれば上書き）。書き込みに失敗した場合は false を返す。
	*/
	Bool MaterialLoader::Bake(const BakedSurface& bakedSurface, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<BakedSurface&>(bakedSurface).Serialize(archive);
		return archive.Write(filePath);
	}
}
