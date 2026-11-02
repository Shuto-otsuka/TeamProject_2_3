#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	class Crister;

	enum class MeshCollisionDetail;

	/**
	* [EN]
	* Loads ".collision" files into MeshCollision objects. Baking a
	* collision mesh out of a Crister is private and reached only by
	* MeshCollisionResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".collision" ファイルを MeshCollision へ読み込む。Crister から
	* コリジョンメッシュを焼き出す処理は private で、MeshCollisionResource
	* からだけ使う。
	*/
	class SEEDCORE_API MeshCollisionLoader :public AssetLoader<MeshCollision>
	{
	private:
		friend class MeshCollisionResource;

	public:
		MeshCollisionLoader() = default;
		~MeshCollisionLoader() = default;

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
		Handle<MeshCollision> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Destroys the collision mesh handle names.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すコリジョンメッシュを破棄する。
		*/
		void Clear(Handle<MeshCollision>& handle)noexcept;

	private:
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
		Bool Bake(const Crister& crister, MeshCollisionDetail detail, String filePath);
	};
}
