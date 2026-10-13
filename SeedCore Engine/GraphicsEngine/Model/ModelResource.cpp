#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/ModelExporter.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	ModelResource::ModelResource(GraphicsLoaderSystem& loaders, BindlessHeap& heap) :loaders_(loaders), heap_(heap)
	{
		/// No Code
	}

	void ModelResource::Load(const AssetContext& context, Uint32 assetID)
	{
		if (!context.bc7Shader_)
		{
			return;
		}

		Load(context.device_, context.cmdQueue_, *context.bc7Shader_, context.cache_, assetID);

		AssetRecord* asset = context.cache_.GetAsset(assetID);
		if (asset)
		{
			loaders_.animationLoader_->SplitClips(asset->fullpath_);
		}
	}

	Handle<Crister> ModelResource::Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, Uint32 assetID)
	{
		if (assetHandleMap_.contains(assetID))
		{
			return assetHandleMap_.at(assetID);
		}

		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return Handle<Crister>::null();
		}

		AxisConvention axisConvention = cache.ReadAxisConvention(assetID);
		Handle<Crister> handle = loaders_.modelLoader_->Load(device, cmdQueue, &heap_, bc7Shader, asset->fullpath_, axisConvention);
		if (handle.empty())
		{
			return Handle<Crister>::null();
		}

		assetHandleMap_.insert({ assetID, handle });

		if (Crister* crister = loaders_.modelLoader_->Get(handle))
		{
			const DynamicArray<Surface>& materials = crister->Surfaces();
			std::filesystem::path modelPath(asset->fullpath_.c_str());
			std::filesystem::path directory = modelPath.parent_path() / (modelPath.stem().string() + ".Materials");
			std::error_code error;
			std::filesystem::create_directories(directory, error);

			std::set<std::string> usedNames;
			for (Size materialIndex = 0; materialIndex < materials.size(); materialIndex++)
			{
				const Surface& material = materials[materialIndex];

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
				if (std::filesystem::exists(filePath))
				{
					continue;
				}
				loaders_.materialLoader_->Save(material, String(filePath.string()));
			}

			if (!crister->Skins().empty())
			{
				std::filesystem::path skeletonPath = modelPath.parent_path() / (modelPath.stem().string() + ".skeleton");
				if (!std::filesystem::exists(skeletonPath))
				{
					SkeletonRig rig;
					loaders_.skeletonLoader_->Populate(rig, assetID);
					loaders_.skeletonLoader_->Save(rig, String(skeletonPath.string()));
				}
			}
		}

		return handle;
	}

	void ModelResource::Unload(Uint32 assetID)
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return;
		}

		Handle<Crister> handle = assetHandleMap_.at(assetID);
		loaders_.modelLoader_->Clear(handle);
		assetHandleMap_.erase(assetID);
	}

	Bool ModelResource::Export(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, Uint32 assetId, ExportPreset preset, String outputPath)
	{
		AssetRecord* asset = cache.GetAsset(assetId);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path exportPath(outputPath.c_str());
		if (exportPath.empty() || modelPath.lexically_normal() == exportPath.lexically_normal())
		{
			return false;
		}

		AxisConvention axisConvention = cache.ReadAxisConvention(assetId);
		Handle<Crister> handle = loaders_.modelLoader_->Load(device, cmdQueue, &heap_, bc7Shader, asset->fullpath_, axisConvention, true);
		Crister* crister = loaders_.modelLoader_->Get(handle);
		if (!crister)
		{
			return false;
		}

		ModelExporter exporter;
		Bool result = exporter.Export(*crister, ModelExporter::Preset(preset), outputPath);

		loaders_.modelLoader_->Clear(handle);
		return result;
	}

	Bool ModelResource::GenerateCollision(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, Uint32 assetID, MeshCollisionDetail detail)
	{
		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] The bake reads CPU-resident cluster data off the Crister, so the model has to be loaded first - Load() is a no-op when it already is.
		/// [JP] ベイクは Crister の CPU 常駐クラスタデータを読むため、先にモデルをロードしておく必要がある - 既にロード済みなら Load() は何もしない。
		Handle<Crister> handle = Load(device, cmdQueue, bc7Shader, cache, assetID);
		Crister* crister = loaders_.modelLoader_->Get(handle);
		if (!crister)
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path collisionPath = modelPath.parent_path() / (modelPath.stem().string() + ((detail == MeshCollisionDetail::Exact) ? ".exact.collision" : ".proxy.collision"));

		return loaders_.meshCollisionLoader_->Bake(*crister, detail, String(collisionPath.string()));
	}

	Bool ModelResource::GenerateMaterial(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, Uint32 assetID, Bool overwrite)
	{
		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return false;
		}

		Handle<Crister> handle = Load(device, cmdQueue, bc7Shader, cache, assetID);
		Crister* crister = loaders_.modelLoader_->Get(handle);
		if (!crister)
		{
			return false;
		}

		const DynamicArray<Surface>& materials = crister->Surfaces();
		if (materials.empty())
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path directory = modelPath.parent_path() / (modelPath.stem().string() + ".Materials");
		std::error_code error;
		std::filesystem::create_directories(directory, error);

		std::set<std::string> usedNames;
		Bool allWritten = true;
		for (Size materialIndex = 0; materialIndex < materials.size(); materialIndex++)
		{
			const Surface& material = materials[materialIndex];

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
			if (!overwrite && std::filesystem::exists(filePath))
			{
				continue;
			}
			if (!loaders_.materialLoader_->Save(material, String(filePath.string())))
			{
				allWritten = false;
			}
		}

		return allWritten;
	}

	Bool ModelResource::GenerateSkeleton(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, Uint32 assetID, Bool overwrite)
	{
		AssetRecord* asset = cache.GetAsset(assetID);
		if (!asset)
		{
			return false;
		}

		Handle<Crister> handle = Load(device, cmdQueue, bc7Shader, cache, assetID);
		Crister* crister = loaders_.modelLoader_->Get(handle);
		if (!crister || crister->Skins().empty())
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path skeletonPath = modelPath.parent_path() / (modelPath.stem().string() + ".skeleton");
		if (!overwrite && std::filesystem::exists(skeletonPath))
		{
			return true;
		}

		SkeletonRig rig;
		loaders_.skeletonLoader_->Populate(rig, assetID);
		return loaders_.skeletonLoader_->Save(rig, String(skeletonPath.string()));
	}

	Handle<Crister> ModelResource::GetHandle(Uint32 assetID)const
	{
		if (!assetHandleMap_.contains(assetID))
		{
			return Handle<Crister>::null();
		}
		return assetHandleMap_.at(assetID);
	}

	Crister* ModelResource::Resolve(const Handle<Crister>& handle)
	{
		return loaders_.modelLoader_->Get(handle);
	}

	Bool ModelResource::Contains(Uint32 assetID)const
	{
		return assetHandleMap_.contains(assetID);
	}
}
