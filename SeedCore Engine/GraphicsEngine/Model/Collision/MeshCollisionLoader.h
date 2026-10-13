#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	class Crister;
	enum class MeshCollisionDetail;

	class SEEDCORE_API MeshCollisionLoader :public NonCopyable
	{
	public:
		MeshCollisionLoader() = default;
		~MeshCollisionLoader() = default;

		Handle<MeshCollision> Load(String filePath);

		MeshCollision* Get(const Handle<MeshCollision>& handle);

		void Clear(Handle<MeshCollision>& handle)noexcept;

		Bool Bake(const Crister& crister, MeshCollisionDetail detail, String filePath);

	private:
		StablePool<MeshCollision> pool_;
	};
}
