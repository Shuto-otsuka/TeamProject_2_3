#pragma once

namespace SeedCore
{
	class SEEDCORE_API MeshCollision :public NonCopyable
	{
	private:
		friend class MeshCollisionLoader;

		DynamicArray<Vector3> positions_;
		DynamicArray<Uint32> indices_;

	public:
		MeshCollision() = default;
		~MeshCollision() = default;

		MeshCollision(MeshCollision&&)noexcept = default;
		MeshCollision& operator=(MeshCollision&&)noexcept = default;

		NODISCARD const DynamicArray<Vector3>& Positions()const;
		NODISCARD const DynamicArray<Uint32>& Indices()const;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("positions", positions_);
			archive.Field("indices", indices_);
		}
	};
}
