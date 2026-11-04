#include "OrangeAreaController.h"

#include <SeedCore/ScScene.h>
#include <SeedCore/ScComponent.h>
#include <SeedCore/ScLog.h>

using namespace SeedCore;

void OrangeAreaController::OnStart()
{
	ownerMaterialID = Scene::AssetID("area_M_L_O.material");
	transparentMaterialID = Scene::AssetID("area_M_L_OT.material");

	fadeTimer_ = 0.0f;

	isTransparent = 1;
}

void OrangeAreaController::OnTick(float elapsedTime)
{
	SC_LOG_NOTICE("通っているよん");
	SC_LOG_NOTICE("通っているよん");
	Material* material = GetActor().GetComponent<Material>();
	if (!material)
	{
		SC_LOG_NOTICE("ないわぼけ");
		return;
	}

	fadeTimer_ += elapsedTime;

	SC_LOG_NOTICE("{}", std::to_string(fadeTime_).c_str());
	SC_LOG_NOTICE("通っているよん");

	if (fadeTimer_ > switchTime_)
	{
		fadeTimer_ = 0.0f;
		isTransparent *= -1;
		SC_LOG_NOTICE("変わったよ～ん");
	}

	float fadeFactor = fadeTimer_ / fadeTime_;

	if (isTransparent == 1)
	{
		material->Set(transparentMaterialID, "bullet_M", fadeFactor);
	}
	else if (isTransparent == -1)
	{
		material->Set(ownerMaterialID, "bullet_M", fadeFactor);
	}
}