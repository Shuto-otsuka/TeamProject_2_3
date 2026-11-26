#include <GraphicsEngine/System/ColliderSystem.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Collider/CapsuleCollider.h>
#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Collider/CircleCollider.h>
#include <PhysicsEngine/CharacterController/CharacterController.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>

namespace SeedCore
{
	void ColliderSystem::Update(World& world)
	{
		colliders_.clear();

		const Color colliderDebugColor(0.0f, 1.0f, 0.0f, 1.0f);

		const Color characterDebugColor(1.0f, 0.0f, 1.0f, 1.0f);

		DynamicArray<EntityID> boxColliderEntities;
		Query<Read<BoxCollider>>(world).ForEach([&](EntityID entityID, const BoxCollider&) { boxColliderEntities.push_back(entityID); });
		for (EntityID id : boxColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			BoxCollider* collider = actor.GetComponent<BoxCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			Vector3 size(collider->size_.x * Abs(worldScale.x), collider->size_.y * Abs(worldScale.y), collider->size_.z * Abs(worldScale.z));
			Vector3 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y, collider->center_.z * worldScale.z);

			colliders_.push_back({ ShapeKind::Box, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition + Vector3::Transform(center, actorRotation), actorRotation, size * 0.5f, colliderDebugColor });
		}

		DynamicArray<EntityID> sphereColliderEntities;
		Query<Read<SphereCollider>>(world).ForEach([&](EntityID entityID, const SphereCollider&) { sphereColliderEntities.push_back(entityID); });
		for (EntityID id : sphereColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			SphereCollider* collider = actor.GetComponent<SphereCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.y), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, 0.0f, 0.0f), colliderDebugColor });
		}

		DynamicArray<EntityID> capsuleColliderEntities;
		Query<Read<CapsuleCollider>>(world).ForEach([&](EntityID entityID, const CapsuleCollider&) { capsuleColliderEntities.push_back(entityID); });
		for (EntityID id : capsuleColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CapsuleCollider* collider = actor.GetComponent<CapsuleCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			Float height = collider->height_ * Abs(worldScale.y);
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, height * 0.5f, 0.0f), colliderDebugColor });
		}

		DynamicArray<EntityID> characterControllerEntities;
		Query<Read<CharacterController>>(world).ForEach([&](EntityID entityID, const CharacterController&) { characterControllerEntities.push_back(entityID); });
		for (EntityID id : characterControllerEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CharacterController* controller = actor.GetComponent<CharacterController>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			Float height = controller->Crouching() ? controller->crouchHeight_ : controller->height_;
			Float radius = controller->radius_;

			Vector3 center(0.0f, height * 0.5f + radius, 0.0f);

			colliders_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition + Vector3::Transform(center, actorRotation), actorRotation, Vector3(radius, height * 0.5f, 0.0f), characterDebugColor });
		}

		DynamicArray<EntityID> cylinderColliderEntities;
		Query<Read<CylinderCollider>>(world).ForEach([&](EntityID entityID, const CylinderCollider&) { cylinderColliderEntities.push_back(entityID); });
		for (EntityID id : cylinderColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CylinderCollider* collider = actor.GetComponent<CylinderCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			Float height = collider->height_ * Abs(worldScale.y);
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Cylinder, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, height * 0.5f, 0.0f), colliderDebugColor });
		}

		DynamicArray<EntityID> rectColliderEntities;
		Query<Read<RectCollider>>(world).ForEach([&](EntityID entityID, const RectCollider&) { rectColliderEntities.push_back(entityID); });
		for (EntityID id : rectColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			RectCollider* collider = actor.GetComponent<RectCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Vector3 pixelPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
			Float pixelX = pixelPosition.x;
			Float pixelY = pixelPosition.y;
			Float angle = ((rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation).ToEuler().z;
			Float cosAngle = std::cos(angle);
			Float sinAngle = std::sin(angle);

			Vector2 size(collider->size_.x * Abs(worldScale.x), collider->size_.y * Abs(worldScale.y));
			Vector2 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y);

			Vector3 instancePosition(100000.0f + pixelX + center.x * cosAngle - center.y * sinAngle, 100000.0f + (ScResolution::SC_CANVAS.Height - pixelY) - center.x * sinAngle - center.y * cosAngle, 100000.0f);
			colliders_.push_back({ ShapeKind::Rect, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, instancePosition, Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -angle), Vector3(size.x * 0.5f, size.y * 0.5f, 0.0f), colliderDebugColor });
		}

		DynamicArray<EntityID> circleColliderEntities;
		Query<Read<CircleCollider>>(world).ForEach([&](EntityID entityID, const CircleCollider&) { circleColliderEntities.push_back(entityID); });
		for (EntityID id : circleColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CircleCollider* collider = actor.GetComponent<CircleCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Vector3 pixelPosition = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
			Float pixelX = pixelPosition.x;
			Float pixelY = pixelPosition.y;
			Float angle = ((rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation).ToEuler().z;
			Float cosAngle = Cos(angle);
			Float sinAngle = Sin(angle);

			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.y));
			Vector2 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y);

			Vector3 instancePosition(100000.0f + pixelX + center.x * cosAngle - center.y * sinAngle, 100000.0f + (ScResolution::SC_CANVAS.Height - pixelY) - center.x * sinAngle - center.y * cosAngle, 100000.0f);
			colliders_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, instancePosition, Quaternion::Identity, Vector3(radius, 0.0f, 0.0f), colliderDebugColor });
		}
	}

	std::span<const ShapeDesc> ColliderSystem::Colliders()const
	{
		return colliders_;
	}
}
