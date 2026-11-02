#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class TextureResource;
	class ModelResource;
	class D3D12Context;
	class BindlessHeap;
	class BC7CompressShader;

	/**
	* [EN]
	* Manages the mapping between asset IDs and loaded ".material" handles:
	* Load by asset ID, retrieve handles, resolve to BakedSurface pointers,
	* unload when no longer needed. Textures baked into a ".material" are
	* handed to TextureResource under the asset IDs the Surface refers to
	* them by, so the renderers resolve them the same way as standalone
	* texture assets. Generate writes a model's ".material" files, and
	* Bake writes an edited material back with its textures re-baked.
	* Which Surface an actor's slot actually draws with is decided by
	* MaterialSystem, not here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID とロード済み ".material" ハンドル間のマッピングを管理する。
	* アセット ID でロード、ハンドル取得、BakedSurface ポインタへの解決、
	* 不要時のアンロード。".material" に焼き込まれたテクスチャは、Surface
	* が参照するアセット ID で TextureResource へ渡すので、レンダラーは
	* 単体のテクスチャアセットと同じ方法で解決する。Generate はモデルの
	* ".material" ファイルを書き、Bake は編集したマテリアルをテクスチャを
	* 焼き直して書き戻す。アクターのスロットが実際にどの Surface で描かれる
	* かは、ここではなく MaterialSystem が決める。
	*/
	class SEEDCORE_API MaterialResource :public AssetResource<BakedSurface>
	{
	public:
		/**
		* [EN]
		* Takes the loaders, the texture manager baked textures are handed
		* to, the model manager a material is generated from, and the GPU
		* objects textures are uploaded and BC7-compressed with.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダー、焼き込まれたテクスチャを渡すテクスチャのマネージャ、
		* マテリアルの生成元になるモデルのマネージャ、テクスチャの
		* アップロードと BC7 圧縮に使う GPU のオブジェクトを受け取る。
		*/
		MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource, ModelResource& modelResource, D3D12Context& context, BindlessHeap& heap, BC7CompressShader& bc7Shader);
		~MaterialResource() = default;

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
		* or a null handle when the asset is unknown or cannot be read. The
		* textures baked into the file are uploaded on thread's transfer
		* lane and handed to TextureResource; a texture already loaded from
		* its own file is kept instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID がまだ読み込まれていなければ読み込み、そのハンドルを返す。
		* 未知のアセットか読めない場合は null のハンドルを返す。ファイルに
		* 焼き込まれたテクスチャは thread の転送レーンでアップロードして
		* TextureResource へ渡す。自分のファイルから読み込み済みのテクスチャが
		* あれば、そちらを残す。
		*/
		Handle<BakedSurface> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		/**
		* [EN]
		* Releases the material loaded under assetID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているマテリアルを解放する。
		*/
		void Unload(Uint32 assetID)override;

		/**
		* [EN]
		* Writes one ".material" file per Crister material slot of the model
		* sourceAssetID into "<model dir>/<model stem>.Materials/", named
		* after each Surface's name_, overwriting existing ones from the
		* current embedded material. Returns false when the model has no
		* materials or a write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデル sourceAssetID の Crister のマテリアルスロット1つにつき1つの
		* ".material" ファイルを "<モデルのディレクトリ>/<モデル名>.Materials/"
		* へ、各 Surface の name_ を名前にして書く。既存のファイルは現在の
		* 内蔵マテリアルで上書きする。モデルにマテリアルが無い、または書き
		* 込みに失敗した場合は false を返す。
		*/
		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		/**
		* [EN]
		* Writes the material loaded under assetID to filePath: its Surface
		* as edited in memory, the textures baked into its file so far plus
		* any added in memory, and every texture asset the Surface refers to
		* re-baked from its current file. Textures the Surface no longer
		* refers to are dropped.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で読み込まれているマテリアルを filePath へ書き出す。中身は
		* メモリ上で編集した Surface、これまでファイルに焼き込まれていた
		* テクスチャとメモリ上で足されたテクスチャ、そして Surface が参照する
		* テクスチャアセットを今のファイルから焼き直したもの。Surface が
		* もう参照していないテクスチャは落とす。
		*/
		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		/**
		* [EN]
		* Resolves a handle to a BakedSurface pointer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルを BakedSurface ポインタに解決する。
		*/
		BakedSurface* Resolve(const Handle<BakedSurface>& handle);

	private:
		/// [EN] Loaders this manager reads and writes ".material" files through.
		/// [JP] このマネージャが ".material" ファイルを読み書きするのに使うローダー。
		GraphicsLoaderSystem& loaders_;

		/// [EN] Texture manager the textures baked into a material are handed to.
		/// [JP] マテリアルに焼き込まれたテクスチャを渡すテクスチャのマネージャ。
		TextureResource& textureResource_;

		/// [EN] Model manager owning the Crister a material is generated from.
		/// [JP] マテリアルの生成元になる Crister を持つモデルのマネージャ。
		ModelResource& modelResource_;

		/// [EN] Device context baked textures are uploaded with.
		/// [JP] 焼き込まれたテクスチャをアップロードするデバイスのコンテキスト。
		D3D12Context& context_;

		/// [EN] Bindless heap the baked textures' shader resource views live in.
		/// [JP] 焼き込まれたテクスチャのシェーダーリソースビューが置かれるバインドレスヒープ。
		BindlessHeap& heap_;

		/// [EN] Compute shader that BC7-compresses the textures Bake re-bakes.
		/// [JP] Bake が焼き直すテクスチャを BC7 圧縮するコンピュートシェーダー。
		BC7CompressShader& bc7Shader_;
	};
}
