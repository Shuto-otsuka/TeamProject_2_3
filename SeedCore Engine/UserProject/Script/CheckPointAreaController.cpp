#include "UserProject/Script/CheckPointAreaController.h"

#include<SeedCore/ScComponent.h>
#include <SeedCore/ScScene.h>

void CheckPointAreaController::OnStart()
{
    onMaterialId = SeedCore::Scene::AssetID("checkpoint_M_E.material");
}

void CheckPointAreaController::OnTick(float elapsedTime)
{
}

void CheckPointAreaController::SetOn()
{ 
    if (isOn)return;

    //光らせる
    isOn = true;
    SeedCore::Material* material = GetActor().GetComponent<SeedCore::Material>();
    material->Set(onMaterialId, "checkpoint_M");
}
