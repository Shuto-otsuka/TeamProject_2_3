#include <GraphicsEngine/Model/Skeleton/SkeletonLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
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

	void SkeletonLoader::Clear(Handle<SkeletonRig>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	void SkeletonLoader::Populate(SkeletonRig& rig, Uint32 sourceModelID)
	{
		rig.sourceModelID_ = sourceModelID;
		rig.rootBoneName_.clear();
		rig.sockets_.clear();
	}

	Bool SkeletonLoader::Bake(const SkeletonRig& rig, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<SkeletonRig&>(rig).Serialize(archive);
		return archive.Write(filePath);
	}
}
