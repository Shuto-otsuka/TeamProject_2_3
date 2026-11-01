#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads and saves standalone ".material" assets - one serialized
	* BakedSurface per file: the Surface (PBR factors + KHR_materials_* +
	* shading model + alpha/double-sided + texture references) plus the
	* textures baked into the file by MaterialBaker. Mirrors
	* MeshCollisionLoader: a pool of BakedSurface, binary (de)serialization
	* through BinaryArchive, no GPU work.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単体 ".material" アセットの読み書きを行う。1 ファイルに BakedSurface
	* を 1 個持つ: Surface（PBR ファクタ + KHR_materials_* + シェーディング
	* モデル + アルファ/両面 + テクスチャの参照）と、MaterialBaker が
	* ファイルに焼き込んだテクスチャ。MeshCollisionLoader と同型:
	* BakedSurface のプール、BinaryArchive でのバイナリ直列化、GPU 処理なし。
	*/
	class SEEDCORE_API MaterialLoader :public NonCopyable
	{
	public:
		MaterialLoader() = default;
		~MaterialLoader() = default;

		Handle<BakedSurface> Load(String filePath);

		BakedSurface* Get(const Handle<BakedSurface>& handle);

		void Clear(Handle<BakedSurface>& handle)noexcept;

		/**
		* [EN]
		* Serializes bakedSurface to filePath as a ".material" binary file
		* (overwriting any existing one). Returns false when the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* bakedSurface を ".material" バイナリファイルとして filePath へ
		* 直列化する（既存があれば上書き）。書き込みに失敗した場合は false を返す。
		*/
		Bool Save(const BakedSurface& bakedSurface, String filePath);

	private:
		StablePool<BakedSurface> pool_;
	};
}
