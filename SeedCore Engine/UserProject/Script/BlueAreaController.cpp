#include "BlueAreaController.h"

#include <SeedCore/ScScene.h>
#include <SeedCore/ScComponent.h>

using namespace SeedCore;

void BlueAreaController::OnStart()
{
	ownerMaterialID = Scene::AssetID("area_M_L_B.material");
	transparentMaterialID = Scene::AssetID("area_M_L_BT.material");

	fadeTimer_ = 0.0f;

	isTransparent = 1;
}

void BlueAreaController::OnTick(float elapsedTime)
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