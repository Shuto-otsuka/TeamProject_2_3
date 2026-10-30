#include "HintSystem.h"
#include "StopController.h"

#include <SeedCore/ScComponent.h>
//#include <SeedCore/ScLog.h>

using namespace SeedCore;

void HintSystem::OnStart()
{
	timer_ = 0.0f;

	Actor actor = GetActor();
	stopController_ = actor.GetComponent<StopController>();

	Text* hintText = actor.GetComponent<Text>();
	if (hintText)
	{
		hintText->color_.a = 0.0f;
	}
}
	
void HintSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Rotation* rotation = actor.GetComponent<Rotation>();
	Vector3 upAxis = Vector3::Up;
	Quaternion targetQuaternion = Quaternion::CreateFromAxisAngle(upAxis, rotateSpeed_ * elapsedTime);
	Transform::Quat(*rotation, Transform::Quat(*rotation) * targetQuaternion);

	Text* hintText = actor.GetComponent<Text>();

	if (!hintText || !stopController_)
	{
		return;
	}

	if (stopController_->IsStop())
	{
		timer_ = 0.0f;
		hintText->text_ = hintText_;
		hintText->color_.a = 1.0f;
		return;
	}

	timer_ += elapsedTime;

	if (timer_ <= maintainTime_)
	{
		return;
	}

	float fadeRate = (timer_ - maintainTime_) / fadeTime_;

	if (fadeRate > 1.0f)
	{
		fadeRate = 1.0f;
	}

	hintText->color_.a = 1.0f - fadeRate;
}