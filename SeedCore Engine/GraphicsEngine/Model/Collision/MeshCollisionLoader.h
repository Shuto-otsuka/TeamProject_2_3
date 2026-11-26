#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	class Crister;

	enum class MeshCollisionDetail;

	class SEEDCORE_API MeshCollisionLoader :public AssetLoader<MeshCollision>
	{
	private:
		friend class MeshCollisionResource;

	public:
		MeshCollisionLoader() = default;
		~MeshCollisionLoader() = default;

		Handle<MeshCollision> Load(String filePath, LoadThread thread);

		void Clear(Handle<MeshCollision>& handle)noexcept;

	private:
		Bool Bake(const Crister& crister, MeshCollisionDetail detail, String filePath);
	};
}
