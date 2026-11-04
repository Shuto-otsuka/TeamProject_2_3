#include "OrangeAreaController.h"

#include <SeedCore/ScScene.h>
#include <SeedCore/ScComponent.h>

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
	Material* material = GetActor().GetComponent<Material>();
	if (!material)
	{
		return;
	}

	if (fadeTimer_ > switchTime_)
	{
		fadeTimer_ = 0.0f;
		isTransparent *= -1;
	}

	if (isTransparent == 1)
	{
		material->Set(transparentMaterialID, "bullet_M", fadeTime_);
	}
	else if (isTransparent == -1)
	{
		material->Set(ownerMaterialID, "bullet_M", fadeTime_);
	}
}