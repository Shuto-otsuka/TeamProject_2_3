#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/Material/MaterialBaker.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
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
	MaterialResource::MaterialResource(GraphicsLoaderSystem& loaders, TextureResource& textureResource, ModelResource& modelResource, D3D12Context& context, BindlessHeap& heap, BC7CompressShader& bc7Shader) :loaders_(loaders), textureResource_(textureResource), modelResource_(modelResource), context_(context), heap_(heap), bc7Shader_(bc7Shader)
	{
		/// No Code
	}

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
	void MaterialResource::Load(const AssetContext& context, Uint32 assetID)
	{
		Load(context.cache_, assetID, context.thread_);
	}

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
	Handle<BakedSurface> MaterialResource::Load(ResourceCache& cache, Uint32 assetID, LoadThread thread)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<BakedSurface>::null();
		}

		Handle<BakedSurface> handle = loaders_.materialLoader_->Load(asset->fullpath_, thread);
		if (handle.empty())
		{
			return Handle<BakedSurface>::null();
		}

		if (BakedSurface* bakedSurface = loaders_.materialLoader_->Get(handle))
		{
			Size textureCount = Min(bakedSurface->textureAssetIDs_.size(), bakedSurface->textures_.size());
			for (Size textureIndex = 0; textureIndex < textureCount; textureIndex++)
			{
				Uint32 textureAssetID = bakedSurface->textureAssetIDs_[textureIndex];

				/// [EN] A texture loaded from its own file is the original and wins over a copy baked into a material; an earlier baked copy is replaced by this one.
				/// [JP] 自分のファイルから読み込んだテクスチャが原本で、マテリアルに焼き込まれた写しより優先する。以前に焼き込まれた写しは、これで置き換える。
				if (textureResource_.Contains(textureAssetID))
				{
					Texture* existing = textureResource_.Resolve(textureResource_.GetHandle(textureAssetID));
					if (existing && !existing->FilePath().view().empty())
					{
						continue;
					}
					textureResource_.Unload(textureAssetID);
				}

				Texture texture = TextureCreator::CreateTextureBitmap(context_.GetDevice(), context_.GetCopyQueue(), context_.GetLoadTransferLane(thread), &heap_, bakedSurface->textures_[textureIndex]);
				if (!texture.Resource())
				{
					continue;
				}
				textureResource_.Adopt(textureAssetID, loaders_.textureLoader_->Adopt(std::move(texture)));
			}

			/// [EN] The baked pixels now live on the GPU, so the CPU copy is released; Bake reads them back from the file when it needs them.
			/// [JP] 焼き込まれたピクセルは GPU 上に移ったので、CPU 側の写しは解放する。Bake が必要とするときはファイルから読み直す。
			bakedSurface->textures_.clear();
			bakedSurface->textures_.shrink_to_fit();
		}

		assetHandleMap_.insert({ assetID, handle });
		revision_++;
		return handle;
	}

	/**
	* [EN]
	* Releases the material loaded under assetID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で読み込まれているマテリアルを解放する。
	*/
	void MaterialResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<BakedSurface> handle = assetHandleMap_.at(assetID);
		loaders_.materialLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
		revision_++;
	}

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
	Bool MaterialResource::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		AssetRecord* asset = context.cache_.GetAsset(sourceAssetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] The slots come from the model's Crister, so the model is loaded first; this does nothing when it already is.
		/// [JP] スロットはモデルの Crister から取るため、先にモデルを読み込む。読み込み済みなら何もしない。
		Crister* crister = modelResource_.Resolve(modelResource_.Load(context.cache_, sourceAssetID, context.thread_));
		if (!crister)
		{
			return false;
		}

		std::span<const Surface> materials = crister->Surfaces();
		if (materials.empty())
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path directory = modelPath.parent_path() / (modelPath.stem().string() + ".Materials");
		std::error_code error;
		std::filesystem::create_directories(directory, error);

		/// [EN] Names already handed out in this model, so two slots with the same name do not write the same file.
		/// [JP] このモデルで既に使った名前。同じ名前のスロット2つが同じファイルへ書かないようにするため。
		std::set<std::string> usedNames;
		Bool allWritten = true;
		for (Size materialIndex = 0; materialIndex < materials.size(); materialIndex++)
		{
			const Surface& material = materials[materialIndex];

			/// [EN] The file is named after the slot, with characters a file name cannot hold replaced; an unnamed slot is named by its index.
			/// [JP] ファイル名はスロットの名前にし、ファイル名に使えない文字は置き換える。名前の無いスロットは番号で名付ける。
			std::string stem = material.name_.empty() ? ("Material_" + std::to_string(materialIndex)) : material.name_;
			for (char& character : stem)
			{
				if (std::string_view("\\/:*?\"<>|").find(character) != std::string_view::npos || static_cast<Uchar>(character) < 0x20)
				{
					character = '_';
				}
			}
			if (usedNames.contains(stem))
			{
				stem += "_" + std::to_string(materialIndex);
			}
			usedNames.insert(stem);

			std::filesystem::path filePath = directory / (stem + ".material");
			BakedSurface bakedSurface;
			bakedSurface.surface_ = material;
			if (!loaders_.materialLoader_->Bake(bakedSurface, String(filePath.string())))
			{
				allWritten = false;
				continue;
			}
			writtenPaths.push_back(String(filePath.string()));
		}

		return allWritten;
	}

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
	Bool MaterialResource::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		BakedSurface* loaded = Resolve(GetHandle(assetID));
		if (!loaded)
		{
			return false;
		}

		/// [EN] The in-memory copy released its baked pixels once they reached the GPU, so the textures baked so far are read back from the material's own file.
		/// [JP] メモリ上の写しは、焼き込んだピクセルが GPU に移った時点で解放しているため、これまで焼き込んだテクスチャはマテリアル自身のファイルから読み直す。
		BakedSurface bakedSurface;
		if (AssetRecord* asset = context.cache_.GetAsset(assetID))
		{
			BinaryInputArchive archive;
			if (archive.Read(asset->fullpath_))
			{
				bakedSurface.Serialize(archive);
			}
		}
		bakedSurface.surface_ = loaded->surface_;

		/// [EN] Textures added in memory since the load (such as ones embedded from the model) replace a same-ID texture from the file, or join it.
		/// [JP] 読み込み後にメモリ上で足されたテクスチャ（モデルから同梱したものなど）は、ファイルにある同じ ID のテクスチャを置き換えるか、新しく加わる。
		Size addedCount = Min(loaded->textureAssetIDs_.size(), loaded->textures_.size());
		for (Size addedIndex = 0; addedIndex < addedCount; addedIndex++)
		{
			auto found = std::ranges::find(bakedSurface.textureAssetIDs_, loaded->textureAssetIDs_[addedIndex]);
			if (found != bakedSurface.textureAssetIDs_.end() && static_cast<Size>(std::distance(bakedSurface.textureAssetIDs_.begin(), found)) < bakedSurface.textures_.size())
			{
				bakedSurface.textures_[std::distance(bakedSurface.textureAssetIDs_.begin(), found)] = loaded->textures_[addedIndex];
			}
			else
			{
				bakedSurface.textureAssetIDs_.push_back(loaded->textureAssetIDs_[addedIndex]);
				bakedSurface.textures_.push_back(loaded->textures_[addedIndex]);
			}
		}

		MaterialBaker baker;
		baker.BakeReferenced(context_.GetDevice(), context_.GetDirectQueue(), bc7Shader_, context.cache_, bakedSurface);
		return loaders_.materialLoader_->Bake(bakedSurface, filePath);
	}

	/**
	* [EN]
	* Resolves a handle to a BakedSurface pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ハンドルを BakedSurface ポインタに解決する。
	*/
	BakedSurface* MaterialResource::Resolve(const Handle<BakedSurface>& handle)
	{
		return loaders_.materialLoader_->Get(handle);
	}
}
