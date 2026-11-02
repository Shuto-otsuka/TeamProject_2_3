#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads standalone ".material" files into BakedSurface objects: the
	* Surface plus the textures baked into the file. Writing a
	* ".material" is private and reached only by MaterialResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単体の ".material" ファイルを BakedSurface（Surface と、ファイルに
	* 焼き込まれたテクスチャ）へ読み込む。".material" の書き出しは private
	* で、MaterialResource からだけ使う。
	*/
	class SEEDCORE_API MaterialLoader :public AssetLoader<BakedSurface>
	{
	private:
		friend class MaterialResource;

	public:
		MaterialLoader() = default;
		~MaterialLoader() = default;

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
		Handle<BakedSurface> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Destroys the material handle names.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すマテリアルを破棄する。
		*/
		void Clear(Handle<BakedSurface>& handle)noexcept;

	private:
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
		Bool Bake(const BakedSurface& bakedSurface, String filePath);
	};
}
