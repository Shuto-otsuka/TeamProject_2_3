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

namespace SeedCore
{
	/**
	* [EN]
	* Rebuilds the collider list from every active Box, Sphere, Capsule,
	* Cylinder, Rect and Circle collider and every CharacterController's
	* capsule in the World.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World の、アクティブな Box・Sphere・Capsule・Cylinder・Rect・Circle の
	* 各コライダーと、各 CharacterController のカプセルから、コライダーの
	* 一覧を作り直す。
	*/
	void ColliderSystem::Update(World& world)
	{
		colliders_.clear();

		const Color colliderDebugColor(0.0f, 1.0f, 0.0f, 1.0f);

		/// [EN] Fully saturated magenta, so character capsules stand out from the green colliders.
		/// [JP] キャラクターのカプセルが緑のコライダーの中で目立つよう、彩度最大のマゼンタにする。
		const Color characterDebugColor(1.0f, 0.0f, 1.0f, 1.0f);

		for (EntityID id : world.GetComponents<BoxCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			BoxCollider* collider = actor.GetComponent<BoxCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			/// [EN] Same scaling as BoxCollider::GetShapeHandle: the size by the absolute scale per axis, the offset by the signed scale.
			/// [JP] BoxCollider::GetShapeHandle と同じ拡縮。サイズは軸ごとのスケールの絶対値、オフセットは符号付きのスケールで掛ける。
			Vector3 size(collider->size_.x * Abs(worldScale.x), collider->size_.y * Abs(worldScale.y), collider->size_.z * Abs(worldScale.z));
			Vector3 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y, collider->center_.z * worldScale.z);

			colliders_.push_back({ ShapeKind::Box, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition + Vector3::Transform(center, actorRotation), actorRotation, size * 0.5f, colliderDebugColor });
		}

		for (EntityID id : world.GetComponents<SphereCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			SphereCollider* collider = actor.GetComponent<SphereCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			/// [EN] Same scaling as SphereCollider::GetShapeHandle: the radius follows the largest axis.
			/// [JP] SphereCollider::GetShapeHandle と同じ拡縮。半径は最も大きい軸に合わせる。
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.y), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, 0.0f, 0.0f), colliderDebugColor });
		}

		for (EntityID id : world.GetComponents<CapsuleCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CapsuleCollider* collider = actor.GetComponent<CapsuleCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			/// [EN] Same scaling as CapsuleCollider::GetShapeHandle: the height follows Y, the radius the larger of X and Z.
			/// [JP] CapsuleCollider::GetShapeHandle と同じ拡縮。高さは Y に、半径は X と Z の大きい方に合わせる。
			Float height = collider->height_ * Abs(worldScale.y);
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, height * 0.5f, 0.0f), colliderDebugColor });
		}

		for (EntityID id : world.GetComponents<CharacterController>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CharacterController* controller = actor.GetComponent<CharacterController>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			/// [EN] Same capsule as Physics::CreateCharacter: the height is the cylinder part, switched to the crouch height while crouched, and Scale is not applied.
			/// [JP] Physics::CreateCharacter と同じカプセル。高さは円柱部分で、しゃがみ中はしゃがみ時の高さに切り替わり、Scale は掛けない。
			Float height = controller->Crouching() ? controller->crouchHeight_ : controller->height_;
			Float radius = controller->radius_;

			/// [EN] The character's origin is its feet, so the capsule center sits half the cylinder plus one radius above it.
			/// [JP] キャラクターの原点は足元なので、カプセルの中心は円柱の半分と半径1つ分だけ上にある。
			Vector3 center(0.0f, height * 0.5f + radius, 0.0f);

			colliders_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition + Vector3::Transform(center, actorRotation), actorRotation, Vector3(radius, height * 0.5f, 0.0f), characterDebugColor });
		}

		for (EntityID id : world.GetComponents<CylinderCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CylinderCollider* collider = actor.GetComponent<CylinderCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Quaternion actorRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

			Vector3 actorPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);

			/// [EN] Same scaling as CylinderCollider::GetShapeHandle: the height follows Y, the radius the larger of X and Z.
			/// [JP] CylinderCollider::GetShapeHandle と同じ拡縮。高さは Y に、半径は X と Z の大きい方に合わせる。
			Float height = collider->height_ * Abs(worldScale.y);
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.z));

			colliders_.push_back({ ShapeKind::Cylinder, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actorPosition, actorRotation, Vector3(radius, height * 0.5f, 0.0f), colliderDebugColor });
		}

		for (EntityID id : world.GetComponents<RectCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			RectCollider* collider = actor.GetComponent<RectCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space, in canvas pixels.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。Canvas のピクセル単位。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Vector3 pixelPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
			Float pixelX = pixelPosition.x;
			Float pixelY = pixelPosition.y;
			Float angle = ((rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation).ToEuler().z;
			Float cosAngle = std::cos(angle);
			Float sinAngle = std::sin(angle);

			/// [EN] Same scaling as RectCollider::GetShapeHandle: only X and Y apply, the size by their absolute values, the offset by the signed ones.
			/// [JP] RectCollider::GetShapeHandle と同じ拡縮。効くのは X と Y だけで、サイズは絶対値、オフセットは符号付きで掛ける。
			Vector2 size(collider->size_.x * Abs(worldScale.x), collider->size_.y * Abs(worldScale.y));
			Vector2 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y);

			Vector3 instancePosition(100000.0f + pixelX + center.x * cosAngle - center.y * sinAngle, 100000.0f + (ScResolution::SC_CANVAS.Height - pixelY) - center.x * sinAngle - center.y * cosAngle, 100000.0f);
			colliders_.push_back({ ShapeKind::Rect, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, instancePosition, Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -angle), Vector3(size.x * 0.5f, size.y * 0.5f, 0.0f), colliderDebugColor });
		}

		for (EntityID id : world.GetComponents<CircleCollider>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CircleCollider* collider = actor.GetComponent<CircleCollider>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();
			const Scale* scale = actor.GetComponent<Scale>();

			/// [EN] Same world pose as PhysicsSystem::ApplyTransform: Position and Rotation carried out of the parent's space, in canvas pixels.
			/// [JP] PhysicsSystem::ApplyTransform と同じワールドでの姿勢: Position と Rotation を親の空間から出したもの。Canvas のピクセル単位。
			Actor parentActor = actor.Parent();
			Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
			Vector3 parentScale;
			Quaternion parentRotation;
			Vector3 parentTranslation;
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
			Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

			Vector3 pixelPosition = Vector3::Transform(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
			Float pixelX = pixelPosition.x;
			Float pixelY = pixelPosition.y;
			Float angle = ((rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation).ToEuler().z;
			Float cosAngle = Cos(angle);
			Float sinAngle = Sin(angle);

			/// [EN] Same scaling as CircleCollider::GetShapeHandle: the radius follows the larger of X and Y, the offset the signed scale.
			/// [JP] CircleCollider::GetShapeHandle と同じ拡縮。半径は X と Y の大きい方に、オフセットは符号付きのスケールに合わせる。
			Float radius = collider->radius_ * Max(Abs(worldScale.x), Abs(worldScale.y));
			Vector2 center(collider->center_.x * worldScale.x, collider->center_.y * worldScale.y);

			Vector3 instancePosition(100000.0f + pixelX + center.x * cosAngle - center.y * sinAngle, 100000.0f + (ScResolution::SC_CANVAS.Height - pixelY) - center.x * sinAngle - center.y * cosAngle, 100000.0f);
			colliders_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, instancePosition, Quaternion::Identity, Vector3(radius, 0.0f, 0.0f), colliderDebugColor });
		}
	}

	/**
	* [EN]
	* Returns the colliders gathered by the last Update.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直前の Update で集めたコライダーを返す。
	*/
	std::span<const ShapeDesc> ColliderSystem::Colliders()const
	{
		return colliders_;
	}
}
