#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>

namespace SeedCore
{
	class SEEDCORE_API SkeletonLoader :public AssetLoader<SkeletonRig>
	{
	private:
		friend class SkeletonResource;

	public:
		SkeletonLoader() = default;
		~SkeletonLoader() = default;

		Handle<SkeletonRig> Load(String filePath, LoadThread thread);

		void Clear(Handle<SkeletonRig>& handle)noexcept;

	private:
		void Populate(SkeletonRig& rig, Uint32 sourceModelID);

		Bool Bake(const SkeletonRig& rig, String filePath);
	};
}
