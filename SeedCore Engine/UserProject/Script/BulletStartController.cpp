#include "UserProject/Script/BulletStartController.h"
#include<SeedCore/ScComponent.h>

void BulletStartController::OnStart()
{
}

void BulletStartController::OnTick(float elapsedTime)
{
}

void BulletStartController::OnTriggerEnter(SeedCore::Entity entity)
{
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    isTouch = true;
}

void BulletStartController::OnTriggerExit(SeedCore::Entity entity)
{
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    isTouch = false;
}
