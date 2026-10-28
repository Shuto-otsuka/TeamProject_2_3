#include "UserProject/Script/AreaController.h"

#include<SeedCore/ScScene.h>

void AreaController::OnStart()
{
    myMaterial = GetActor().GetComponent<SeedCore::Material>();
}

void AreaController::OnTick(float elapsedTime)
{
    radian += fadeSpeed * elapsedTime;
    float rate = sinf(radian);
    myMaterial->Set(lerpMaterial,"[0]bullet_M:bullet_M (2) (3).material",rate);
}
