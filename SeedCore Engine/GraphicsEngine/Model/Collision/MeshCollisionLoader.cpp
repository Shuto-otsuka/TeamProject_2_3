#include <GraphicsEngine/Model/Collision/MeshCollisionLoader.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerExtractor.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads the ".collision" file at filePath into a new MeshCollision
	* and returns its handle, or a null handle when the file is missing,
	* has another extension or cannot be read. thread is unused: a
	* collision mesh needs no per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の ".collision" ファイルを新しい MeshCollision へ読み込み、
	* そのハンドルを返す。ファイルが無い、拡張子が違う、読めない場合は
	* null のハンドルを返す。thread は使わない（コリジョンメッシュは
	* スレッドごとの状態を持たないため）。
	*/
	Handle<MeshCollision> MeshCollisionLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".collision" || !std::filesystem::exists(path))
		{
			return Handle<MeshCollision>::null();
		}

		Handle<MeshCollision> handle = pool_.Create();
		MeshCollision* meshCollision = pool_.Get(handle);
		if (!meshCollision)
		{
			return Handle<MeshCollision>::null();
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			pool_.Destroy(handle);
			return Handle<MeshCollision>::null();
		}
		meshCollision->Serialize(archive);

		return handle;
	}

	/**
	* [EN]
	* Destroys the collision mesh handle names.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle が示すコリジョンメッシュを破棄する。
	*/
	void MeshCollisionLoader::Clear(Handle<MeshCollision>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Flattens crister's triangles at the given detail and writes them to
	* filePath as a ".collision" file. Returns false when no triangle
	* comes out or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* crister の三角形を指定した detail で展開し、".collision" ファイル
	* として filePath へ書き出す。三角形が1つも出ない、または書き込みに
	* 失敗した場合は false を返す。
	*/
	Bool MeshCollisionLoader::Bake(const Crister& crister, MeshCollisionDetail detail, String filePath)
	{
		MeshCollision meshCollision;
		CristerExtractor extractor;
		if (!extractor.Collision(crister, detail, meshCollision.positions_, meshCollision.indices_))
		{
			return false;
		}

		BinaryOutputArchive archive;
		meshCollision.Serialize(archive);
		return archive.Write(filePath);
	}
}
