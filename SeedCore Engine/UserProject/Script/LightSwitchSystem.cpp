#include "UserProject/Script/LightSwitchSystem.h"

#include <SeedCore/ScComponent.h>
#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScScene.h>

using namespace SeedCore;

void LightSwitchSystem::OnStart()
{
	isOn_ = false;

	offMaterialID_ = Scene::AssetID("switch_cube_M.material");
	onMaterialID_ = Scene::AssetID("switch_cube_M_E.material");
}

void LightSwitchSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();

	Rotation* rotation = actor.GetComponent<Rotation>();
	Vector3 upAxis = Vector3::Up;
	Quaternion targetQuaternion = Quaternion::CreateFromAxisAngle(upAxis, rotateSpeed_ * elapsedTime);
	Transform::Quat(*rotation, Transform::Quat(*rotation) * targetQuaternion);

	if (keepResult_ && isOn_)
	{
		return;
	}

	Vector3 position, scale;
	Quaternion quaternion;
	Matrix worldMatrix = actor.WorldMatrix();
	worldMatrix.Decompose(scale, quaternion, position);
	Physics& physics = *actor.GetService<Physics>();
	DynamicArray<EntityID> entityIDs = physics.Overlap(OverlapShapeKind::Sphere, Vector3(detectDistance_, 0.0f, 0.0f), position, Quaternion::Identity);

	bool inside = false;
	for (EntityID entityID : entityIDs)
	{
		Actor other = world.GetActor(entityID);
		if (other && other.HasTag("Player"))
		{
			inside = true;
			break;
		}
	}

	isOn_ = inside;

	Material* material = actor.GetComponent<Material>();
	if (material && onMaterialID_ != 0 && offMaterialID_ != 0)
	{
		material->Set(isOn_ ? onMaterialID_ : offMaterialID_, "switch_cube_M");
	}
}

bool LightSwitchSystem::IsOn()const
{
	return isOn_;
}