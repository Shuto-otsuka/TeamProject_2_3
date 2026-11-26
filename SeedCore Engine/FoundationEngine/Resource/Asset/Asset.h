#pragma once
#include <FoundationEngine/Utility/ResourcePtr.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>

namespace SeedCore
{
	class ResourceCache;

	enum class AssetType
	{
		Texture,

		Model,

		Animation,

		MeshCollision,

		Material,

		Skeleton,

		Effect,

		Effekseer,

		Houdini,

		Audio,

		Font,

		Movie,

		Skymap,

		Prefab,

		Scene,

		Unknown,
	};

	struct AssetRecord
	{
		String path_;

		String fullpath_;

		AssetType type_ = AssetType::Unknown;

		Uint32 assetID_ = 0;

		Bool isLoaded_ = false;
	};

	struct SEEDCORE_API AssetMeta
	{
		Uint32 version_ = 2;

		Uint32 guid_ = 0;

		AxisConvention axisConvention_;

		Matrix modelTransform_ = Matrix::Identity;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("version", version_);
			archive.Field("guid", guid_);
			archive.TryField("axis_convention", axisConvention_);
			archive.TryField("model_transform", modelTransform_);
		}

		Bool Read(const std::filesystem::path& filePath);

		Bool Write(const std::filesystem::path& filePath)const;
	};

	enum class LoadThread
	{
		Main,

		Async
	};

	struct AssetContext
	{
		ResourceCache& cache_;

		LoadThread thread_ = LoadThread::Main;
	};

	class SEEDCORE_API Asset
	{
	public:
		virtual ~Asset() = default;

		virtual void Load(const AssetContext& context, Uint32 assetID) = 0;

		virtual void Unload(Uint32 assetID) = 0;

		virtual void Clear();

		virtual Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths);

		virtual Bool Bake(const AssetContext& context, Uint32 assetID, String filePath);
	};
}
