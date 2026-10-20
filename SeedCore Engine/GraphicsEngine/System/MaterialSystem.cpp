#include <GraphicsEngine/System/MaterialSystem.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	void MaterialSystem::Execute(World& world, MaterialResource& materialResource, ModelResource& modelResource)
	{
		for (EntityID id : world.GetComponents<Material>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			Material* material = world.GetComponent<Material>(actor.GetEntity());
			const Mesh* mesh = world.GetComponent<Mesh>(actor.GetEntity());
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
