#include <GraphicsEngine/Model/Skeleton/SkeletonLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads the ".skeleton" file at filePath into a new rig and returns
	* its handle, or a null handle when the file is missing, has another
	* extension or cannot be read. thread is unused: a rig needs no
	* per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の ".skeleton" ファイルを新しいリグへ読み込み、その
	* ハンドルを返す。ファイルが無い、拡張子が違う、読めない場合は null の
	* ハンドルを返す。thread は使わない（リグはスレッドごとの状態を
	* 持たないため）。
	*/
	Handle<SkeletonRig> SkeletonLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".skeleton" || !std::filesystem::exists(path))
		{
			return Handle<SkeletonRig>::null();
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			return Handle<SkeletonRig>::null();
		}

		Handle<SkeletonRig> handle = pool_.Create();
		SkeletonRig* rig = pool_.Get(handle);
		if (!rig)
		{
			return Handle<SkeletonRig>::null();
		}
		rig->Serialize(archive);

		return handle;
	}

	/**
	* [EN]
	* Destroys the rig handle names.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle が示すリグを破棄する。
	*/
	void SkeletonLoader::Clear(Handle<SkeletonRig>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Fills a fresh rig for a model: links it to sourceModelID and leaves
	* the root bone / sockets empty for the user to author.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデル用に空のリグを初期化する: sourceModelID へ紐づけ、
	* ルートボーン / ソケットはユーザーが編集するよう空のままにする。
	*/
	void SkeletonLoader::Populate(SkeletonRig& rig, Uint32 sourceModelID)
	{
		rig.sourceModelID_ = sourceModelID;
		rig.rootBoneName_.clear();
		rig.sockets_.clear();
	}

	/**
	* [EN]
	* Serializes rig to filePath as a ".skeleton" binary file
	* (overwriting any existing one). Returns false when the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* rig を ".skeleton" バイナリファイルとして filePath へ直列化
	* する（既存があれば上書き）。書き込みに失敗した場合は false を返す。
	*/
	Bool SkeletonLoader::Bake(const SkeletonRig& rig, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<SkeletonRig&>(rig).Serialize(archive);
		return archive.Write(filePath);
	}
}
