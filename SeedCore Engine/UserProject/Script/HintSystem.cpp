#include "HintSystem.h"
#include "HintImageScript.h"

#include <SeedCore/ScLog.h>
#include <SeedCore/ScScene.h>

using namespace SeedCore;

SeedCore::Actor HintSystem::activeTextActor_ = SeedCore::Actor();

void HintSystem::OnStart()
{
	timer_ = maintainTime_ + fadeTime_;

	for (const Actor& child : GetActor().Children())
	{
		Name* name = child.GetComponent<Name>();
		if (!name)
		{
			continue;
		}

		if (name->name_ == "HintText")
		{
			textActor_ = child;
		}
		else if (name->name_ == "HintStand")
		{
			standActor_ = child;
		}
		else if (name->name_ == "HintGlass")
		{
			glassActor_ = child;
		}
	}

	imageActor_ = GetWorld().GetActor("HintImage");

	Text* hintText = textActor_.GetComponent<Text>();
	if (hintText)
	{
		hintText->color_.a = 0.0f;
	}

	offMaterialID_ = Scene::AssetID("hint_glass_M_E.material");
	onMaterialID_ = Scene::AssetID("hint_glass_M.material");
}
	
void HintSystem::OnTick(float elapsedTime)
{
	if (!textActor_ || !standActor_ || !glassActor_ || !imageActor_)
	{
		SC_LOG_ERROR("テキスト用アクター、または、メッシュ用アクターが見つかりませんでした");
		return;
	}

	World& world = GetWorld();
	Actor actor = GetActor();
	Rotation* glassRotation = glassActor_.GetComponent<Rotation>();
	Rotation* standRotation = standActor_.GetComponent<Rotation>();
	Vector3 upAxis = Vector3::Up;
	Quaternion targetQuaternion = Quaternion::CreateFromAxisAngle(upAxis, rotateSpeed_ * elapsedTime);
	Transform::Quat(*glassRotation, Transform::Quat(*glassRotation) * targetQuaternion);
	Transform::Quat(*standRotation, Transform::Quat(*standRotation) * targetQuaternion);
	
	Text* hintText = textActor_.GetComponent<Text>();
	SeedCore::Active* hintActive = textActor_.GetComponent<SeedCore::Active>();

	HintImageScript* hintImageScript = imageActor_.GetComponent<HintImageScript>();

	if (!hintText)
	{
		return;
	}

	if (startup_)
	{
		timer_ = 0.0f;
		if (activeTextActor_)
		{
			SeedCore::Text* beforeText = activeTextActor_.GetComponent<SeedCore::Text>();
			beforeText->color_.a = 0.0f;
		}
		activeTextActor_ = textActor_;
		hintText->color_.a = 1.0f;
		hintImageScript->Display(maintainTime_,fadeTime_);
		startup_ = false;
	}
	else if(hintText->color_.a > 0.0f)
	{
		timer_ += elapsedTime;

		if (timer_ > maintainTime_)
		{
			float fadeRate = (timer_ - maintainTime_) / fadeTime_;

			if (fadeRate > 1.0f)
				fadeRate = 1.0f;

			hintText->color_.a = 1.0f - fadeRate;
		}
	}

	hintActive->active_ = hintText->color_.a > 0.0f;

	Material* material = glassActor_.GetComponent<Material>();
	if (material && onMaterialID_ != 0 && offMaterialID_ != 0)
	{
		material->Set(hintText->color_.a > 0.0f ? onMaterialID_ : offMaterialID_, "hint_glass_M");
	}
}

void HintSystem::Startup()
{
	startup_ = true;
}