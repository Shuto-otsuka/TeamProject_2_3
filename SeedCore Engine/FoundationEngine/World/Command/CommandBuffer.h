#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class World;
	class ResourceCache;

	class SEEDCORE_API CommandBuffer
	{
	public:
		CommandBuffer() = default;

		~CommandBuffer() = default;

		EntityID SpawnPrefab(Uint32 assetID, const Vector3& position, EntityID parent = EntityID{});

		void DestroyEntity(EntityID id);

		template<typename T>
		void AddComponent(EntityID id, const T& value);

		void AddComponent(EntityID id, ComponentID componentID, const void* source);

		void RemoveComponent(EntityID id, ComponentID componentID);

		Bool Empty()const;

		void Flush(World& world, ResourceCache& cache);

		EntityID Resolved(EntityID provisional)const;

		void Clear();

		static Bool Provisional(EntityID id);

	private:
		SC_CONSTEX Uint32 ProvisionalGeneration = 0xFFFFFFFFu;

		enum class Kind : Uint8
		{
			SpawnPrefab,
			DestroyEntity,
			AddComponent,
			RemoveComponent
		};

		struct Command
		{
			Kind kind_ = Kind::SpawnPrefab;

			EntityID target_;

			EntityID parent_;

			ComponentID componentID_ = nullptr;

			Uint32 assetID_ = 0;

			Vector3 position_;

			Size dataOffset_ = 0;
		};

		struct alignas(16) BlobCell
		{
			Byte bytes_[16];
		};

		DynamicArray<Command> commands_;

		DynamicArray<BlobCell> blob_;

		Uint32 provisionalCount_ = 0;

		FlatMap<Uint32, EntityID> resolved_;
	};

	template<typename T>
	void CommandBuffer::AddComponent(EntityID id, const T& value)
	{
		AddComponent(id, ComponentRegistry::GetComponentID<T>(), &value);
	}
}
