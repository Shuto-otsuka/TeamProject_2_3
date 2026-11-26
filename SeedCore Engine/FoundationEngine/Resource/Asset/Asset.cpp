#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	Bool AssetMeta::Read(const std::filesystem::path& filePath)
	{
		std::filesystem::path metaPath = filePath;
		metaPath += ".meta";
		if (!std::filesystem::exists(metaPath))
		{
			return false;
		}

		BinaryInputArchive archive;
		if (!archive.Read(String(metaPath.string())))
		{
			return false;
		}
		Serialize(archive);
		return true;
	}

	Bool AssetMeta::Write(const std::filesystem::path& filePath)const
	{
		std::filesystem::path metaPath = filePath;
		metaPath += ".meta";

		BinaryOutputArchive archive;
		const_cast<AssetMeta*>(this)->Serialize(archive);
		return archive.Write(String(metaPath.string()));
	}

	void Asset::Clear()
	{
		/// No Code
	}

	Bool Asset::Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)
	{
		return true;
	}

	Bool Asset::Bake(const AssetContext& context, Uint32 assetID, String filePath)
	{
		return false;
	}
}
