#include <GraphicsEngine/System/ConstraintSystem.h>
#include <GraphicsEngine/Constraint/PositionConstraint.h>
#include <GraphicsEngine/Constraint/RotationConstraint.h>
#include <GraphicsEngine/Constraint/ParentConstraint.h>
#include <GraphicsEngine/Constraint/LookAtConstraint.h>
#include <GraphicsEngine/Constraint/AttachmentConstraint.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	void ConstraintSystem::Execute(World& world)
	{
		DynamicArray<EntityID> constrainedEntities;

		Query<Read<PositionConstraint>>(world).ForEach([&](EntityID id, const PositionConstraint&) { constrainedEntities.push_back(id); });
		Query<Read<RotationConstraint>>(world).ForEach([&](EntityID id, const RotationConstraint&) { constrainedEntities.push_back(id); });
		Query<Read<ParentConstraint>>(world).ForEach([&](EntityID id, const ParentConstraint&) { constrainedEntities.push_back(id); });
		Query<Read<AttachmentConstraint>>(world).ForEach([&](EntityID id, const AttachmentConstraint&) { constrainedEntities.push_back(id); });
		Query<Read<LookAtConstraint>>(world).ForEach([&](EntityID id, const LookAtConstraint&) { constrainedEntities.push_back(id); });

		if (constrainedEntities.empty())
		{
			return;
		}

		/// [EN] A constrained actor's overridden matrix must propagate to every descendant (even unconstrained ones), so the dirty set includes each constrained actor's whole subtree, not just the constrained actors themselves.
		/// [JP] コンストレイントを持つ actor の上書き済み行列は、全ての子孫(コンストレイント無しも含む)へ伝播する必要があるため、dirty集合はコンストレイントを持つ actor 自身だけでなく、その全サブツリーを含む。
		std::unordered_set<EntityID> dirty;
		for (EntityID id : constrainedEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (actor && actor.Active())
			{
				MarkDirtySubtree(actor, dirty);
			}
		}

		/// [EN] Recompute starting only from each dirty actor whose real parent is not itself dirty, reusing that parent's TransformSystem-computed world matrix as the base.
		/// [JP] dirtyな actor のうち、実際の親が dirty でないものだけを起点に再計算し、その親の TransformSystem 計算済みワールド行列をベースとして再利用する。
		for (EntityID id : dirty)
		{
			Actor actor = WorldAccessor::Make(world, id);
			Actor parent = actor.Parent();
			if (!parent || !dirty.contains(ActorAccessor::GetEntity(parent).GetID()))
			{
				Matrix parentMatrix = parent ? parent.WorldMatrix() : Matrix::Identity;
				UpdateActor(actor, parentMatrix, world, dirty);
			}
		}
	}

	void ConstraintSystem::MarkDirtySubtree(Actor actor, std::unordered_set<EntityID>& dirty)
	{
		if (!dirty.insert(ActorAccessor::GetEntity(actor).GetID()).second)
		{
			return;
		}

		for (Actor child : actor.Children())
		{
			MarkDirtySubtree(child, dirty);
		}
	}

	void ConstraintSystem::UpdateActor(Actor actor, const Matrix& parentMatrix, World& world, const std::unordered_set<EntityID>& dirty)
	{
		Entity entity = ActorAccessor::GetEntity(actor);

		Position* position = world.GetComponent<Position>(entity);
		Rotation* rotation = world.GetComponent<Rotation>(entity);
		Scale* scale = world.GetComponent<Scale>(entity);

		Matrix local = Matrix::Identity;

		if (scale)
		{
			local *= Matrix::CreateScale(scale->x_, scale->y_, scale->z_);
		}

		if (rotation)
		{
			local *= Matrix::CreateFromQuaternion(Transform::Quat(*rotation));
		}

		if (position)
		{
			local *= Matrix::CreateTranslation(position->x_, position->y_, position->z_);
		}

		Matrix worldMatrix = local * parentMatrix;

		Vector3 finalScale;
		Quaternion finalRotation;
		Vector3 finalTranslation;
		worldMatrix.Decompose(finalScale, finalRotation, finalTranslation);

		AttachmentConstraint* attachmentConstraint = world.GetComponent<AttachmentConstraint>(entity);
		ParentConstraint* parentConstraint = world.GetComponent<ParentConstraint>(entity);
		PositionConstraint* positionConstraint = world.GetComponent<PositionConstraint>(entity);
		RotationConstraint* rotationConstraint = world.GetComponent<RotationConstraint>(entity);
		LookAtConstraint* lookAtConstraint = world.GetComponent<LookAtConstraint>(entity);

		const Char* attachmentBoneName = attachmentConstraint ? attachmentConstraint->boneName_.c_str() : nullptr;
		if (attachmentConstraint && attachmentConstraint->enabled_ && attachmentConstraint->target_ != 0 && attachmentBoneName && *attachmentBoneName != '\0')
		{
			Actor target = WorldAccessor::Find(world, attachmentConstraint->target_);
			Skeleton* targetSkeleton = target ? target.GetComponent<Skeleton>() : nullptr;
			if (targetSkeleton && targetSkeleton->HasBone(attachmentConstraint->boneName_))
			{
				Matrix boneWorldMatrix = targetSkeleton->BoneWorldMatrix(attachmentConstraint->boneName_);

				Matrix offsetRotationMatrix = Matrix::CreateFromYawPitchRoll(ToRadians(attachmentConstraint->rotationOffset_.y), ToRadians(attachmentConstraint->rotationOffset_.x), ToRadians(attachmentConstraint->rotationOffset_.z));
				Matrix offsetMatrix = offsetRotationMatrix * Matrix::CreateTranslation(attachmentConstraint->positionOffset_);
				Matrix attachedMatrix = offsetMatrix * boneWorldMatrix;

				Vector3 attachedScale;
				Quaternion attachedRotation;
				Vector3 attachedTranslation;
				if (attachedMatrix.Decompose(attachedScale, attachedRotation, attachedTranslation))
				{
					finalTranslation = Vector3::Lerp(finalTranslation, attachedTranslation, attachmentConstraint->weight_);
					finalRotation = Quaternion::Slerp(finalRotation, attachedRotation, attachmentConstraint->weight_);
				}
			}
		}
		else if (parentConstraint && parentConstraint->enabled_)
		{
			Actor target = (parentConstraint->target_ != 0) ? WorldAccessor::Find(world, parentConstraint->target_) : Actor();
			if (target)
			{
				Matrix offsetRotationMatrix = Matrix::CreateFromYawPitchRoll(ToRadians(parentConstraint->rotationOffset_.y), ToRadians(parentConstraint->rotationOffset_.x), ToRadians(parentConstraint->rotationOffset_.z));
				Matrix offsetMatrix = offsetRotationMatrix * Matrix::CreateTranslation(parentConstraint->positionOffset_);
				Matrix constrainedMatrix = offsetMatrix * target.WorldMatrix();

				Vector3 constrainedScale;
				Quaternion constrainedRotation;
				Vector3 constrainedTranslation;
				if (constrainedMatrix.Decompose(constrainedScale, constrainedRotation, constrainedTranslation))
				{
					finalTranslation = Vector3::Lerp(finalTranslation, constrainedTranslation, parentConstraint->weight_);
					finalRotation = Quaternion::Slerp(finalRotation, constrainedRotation, parentConstraint->weight_);
				}
			}
		}
		else
		{
			if (positionConstraint && positionConstraint->enabled_)
			{
				Actor target = (positionConstraint->target_ != 0) ? WorldAccessor::Find(world, positionConstraint->target_) : Actor();
				if (target)
				{
					Matrix targetWorldMatrix = target.WorldMatrix();
					Vector3 targetScale;
					Quaternion targetRotation;
					Vector3 targetTranslation;
					targetWorldMatrix.Decompose(targetScale, targetRotation, targetTranslation);

					Vector3 desiredTranslation = targetTranslation + positionConstraint->offset_;
					finalTranslation = Vector3::Lerp(finalTranslation, desiredTranslation, positionConstraint->weight_);
				}
			}

			if (rotationConstraint && rotationConstraint->enabled_)
			{
				Actor target = (rotationConstraint->target_ != 0) ? WorldAccessor::Find(world, rotationConstraint->target_) : Actor();
				if (target)
				{
					Matrix targetWorldMatrix = target.WorldMatrix();
					Vector3 targetScale;
					Quaternion targetRotation;
					Vector3 targetTranslation;
					targetWorldMatrix.Decompose(targetScale, targetRotation, targetTranslation);

					Quaternion offsetRotation = Quaternion::CreateFromYawPitchRoll(ToRadians(rotationConstraint->offset_.y), ToRadians(rotationConstraint->offset_.x), ToRadians(rotationConstraint->offset_.z));
					Quaternion desiredRotation = offsetRotation * targetRotation;

					finalRotation = Quaternion::Slerp(finalRotation, desiredRotation, rotationConstraint->weight_);
				}
			}
		}

		if (lookAtConstraint && lookAtConstraint->enabled_)
		{
			Actor target = (lookAtConstraint->target_ != 0) ? WorldAccessor::Find(world, lookAtConstraint->target_) : Actor();
			if (target)
			{
				Matrix targetWorldMatrix = target.WorldMatrix();
				Vector3 targetScale;
				Quaternion targetRotation;
				Vector3 targetTranslation;
				targetWorldMatrix.Decompose(targetScale, targetRotation, targetTranslation);

				Vector3 forward = targetTranslation - finalTranslation;
				if (forward.LengthSquared() > 0.0001f)
				{
					forward.Normalize();
					Matrix lookMatrix = Matrix::CreateWorld(finalTranslation, forward, lookAtConstraint->upVector_);
					Quaternion desiredRotation = Quaternion::CreateFromRotationMatrix(lookMatrix);

					finalRotation = Quaternion::Slerp(finalRotation, desiredRotation, lookAtConstraint->weight_);
				}
			}
		}

		worldMatrix = Matrix::CreateScale(finalScale) * Matrix::CreateFromQuaternion(finalRotation) * Matrix::CreateTranslation(finalTranslation);

		actor.WorldMatrix(worldMatrix);

		for (Actor child : actor.Children())
		{
			if (dirty.contains(ActorAccessor::GetEntity(child).GetID()))
			{
				UpdateActor(child, worldMatrix, world, dirty);
			}
		}
	}
}
