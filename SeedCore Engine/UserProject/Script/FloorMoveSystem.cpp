#include "FloorMoveSystem.h"

#include <SeedCore/ScDebug.h>

#include "StopController.h"

using namespace SeedCore;

void FloorMoveSystem::OnStart()
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	stopController_ = world.GetComponent<StopController>(entity);

	Matrix worldMatrix = actor.WorldMatrix();
	Vector3 floorPosition, floorScale;
	Quaternion floorRotation;
	worldMatrix.Decompose(floorScale, floorRotation, floorPosition);
	cachePosition_ = floorPosition;
	cacheRotation_ = floorRotation;
	startIndex_ = 0;
	pingPongDirection_ = 1;
	progress_ = 0.0f;
	waitTimer_ = 0.0f;
	currentTarget_ = cachePosition_;

	if (static_cast<int>(localPositions_.size()) < 2)
	{
		isMoveLock_ = true;
	}
	else
	{
		isMoveLock_ = false;
	}
}

void FloorMoveSystem::OnFixedTick(float elapsedTime)
{
	if (isMoveLock_)
	{
		return;
	}

	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	Rigidbody* rigidbody = world.GetComponent<Rigidbody>(entity);
	if (!rigidbody)
	{
		return;
	}

	if (stopController_ && stopController_->IsStop())
	{
		rigidbody->MoveTarget(currentTarget_, cacheRotation_, elapsedTime);
		return;
	}

	Vector3 startPosition = localPositions_[startIndex_];
	if (waitTimer_ > 0.0f)
	{
		waitTimer_ -= elapsedTime;

		Vector3 targetPosition = cachePosition_ + startPosition;
		currentTarget_ = targetPosition;
		rigidbody->MoveTarget(targetPosition, cacheRotation_, elapsedTime);
		return;
	}

	int arrivalIndex = startIndex_ + pingPongDirection_;
	if (arrivalIndex < 0 || arrivalIndex >= static_cast<int>(localPositions_.size()))
	{
		isMoveLock_ = true;
		return;
	}
	Vector3 arrivalPosition = localPositions_[arrivalIndex];
	float sectionLength = Vector3::Distance(arrivalPosition, startPosition);
	if (sectionLength < 1e-5f)
	{
		progress_ = 1.0f;
	}
	else
	{
		progress_ += scalarSpeed_ * elapsedTime / sectionLength;
		if (progress_ >= 1.0f)
		{
			progress_ = 1.0f;
		}
	}

	Vector3 targetPosition = cachePosition_ + Vector3::Lerp(startPosition, arrivalPosition, progress_);
	currentTarget_ = targetPosition;
	rigidbody->MoveTarget(targetPosition, cacheRotation_, elapsedTime);

	if (progress_ >= 1.0f)
	{
		startIndex_ = arrivalIndex;
		progress_ = 0.0f;
		switch (moveType_)
		{
		case MoveType::PingPong:
			if ((startIndex_ == static_cast<int>(localPositions_.size()) - 1) || startIndex_ == 0)
			{
				pingPongDirection_ *= -1;
				waitTimer_ = waitTime_;
			}
			break;
		case MoveType::Repeat:
			if (startIndex_ == static_cast<int>(localPositions_.size()) - 1)
			{
				startIndex_ = 0;
				waitTimer_ = waitTime_;
			}
			break;
		}
	}
}

void FloorMoveSystem::OnEditorTick(float elapsedTime)
{
	Vector3 floorPosition = GetActor().WorldMatrix().Translation();
	for (int index = 0;index < static_cast<int>(localPositions_.size());++index)
	{
		GetActor().GetDebugDraw().Sphere(localPositions_[index] + floorPosition, 0.1f, Color(1.0f, 0.0f, 0.0f, 1.0f));
		if (isDebugArrowDraw_)
		{
			if (static_cast<int>(localPositions_.size()) > index + 1)
			{
				Vector3 startPosition = localPositions_[index] + floorPosition;
				Vector3 endPosition = localPositions_[index + 1] + floorPosition;
				GetActor().GetDebugDraw().Arrow(startPosition, endPosition, Color(0.0f, 1.0f, 1.0f, 1.0f), 0.2f);
			}
		}
	}
}