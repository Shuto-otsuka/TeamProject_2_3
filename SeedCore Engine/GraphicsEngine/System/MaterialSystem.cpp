#include <GraphicsEngine/System/MaterialSystem.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>

namespace SeedCore
{
	void MaterialSystem::Execute(World& world, MaterialResource& materialResource, ModelResource& modelResource)
	{
		DynamicArray<EntityID> materialEntities;
		Query<Read<Material>>(world).ForEach([&](EntityID entityID, const Material&) { materialEntities.push_back(entityID); });
		for (EntityID id : materialEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			Material* material = world.GetComponent<Material>(ActorAccessor::GetEntity(actor));
			const Mesh* mesh = world.GetComponent<Mesh>(ActorAccessor::GetEntity(actor));
			if (!material || !mesh || mesh->meshID_ == 0)
			{
				continue;
			}

			Handle<Crister> cristerHandle = modelResource.GetHandle(mesh->meshID_);
			if (cristerHandle.empty())
			{
				continue;
			}

			Crister* crister = modelResource.Resolve(cristerHandle);
			if (!crister)
			{
				continue;
			}

			material->Apply(*crister, cristerHandle, materialResource);
		}
	}
}
