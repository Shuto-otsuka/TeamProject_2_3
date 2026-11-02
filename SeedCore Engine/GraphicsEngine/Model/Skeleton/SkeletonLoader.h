#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads standalone ".skeleton" assets - one serialized SkeletonRig per
	* file (source model id + root bone name + sockets). A pool of
	* SkeletonRig, binary (de)serialization through BinaryArchive, no GPU
	* work. The rig holds only human-authored data - the bone hierarchy and
	* reference pose stay on the Crister. Writing a rig is private and
	* reached only by SkeletonResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単体 ".skeleton" アセット（SkeletonRig 1 個 = ソースモデル ID +
	* ルートボーン名 + ソケット）を読み込む。SkeletonRig のプール、
	* BinaryArchive でのバイナリ直列化、GPU 処理なし。リグが持つのは人が
	* 編集したデータだけ - ボーン階層と参照ポーズは Crister に残る。
	* リグの書き出しは private で、SkeletonResource からだけ使う。
	*/
	class SEEDCORE_API SkeletonLoader :public AssetLoader<SkeletonRig>
	{
	private:
		friend class SkeletonResource;

	public:
		SkeletonLoader() = default;
		~SkeletonLoader() = default;

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
		Handle<SkeletonRig> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Destroys the rig handle names.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すリグを破棄する。
		*/
		void Clear(Handle<SkeletonRig>& handle)noexcept;

	private:
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
		void Populate(SkeletonRig& rig, Uint32 sourceModelID);

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
		Bool Bake(const SkeletonRig& rig, String filePath);
	};
}
