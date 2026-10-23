#include "UserProject/Script/BulletStartController.h"
#include<SeedCore/ScComponent.h>

void BulletStartController::OnStart()
{
}

void BulletStartController::OnTick(float elapsedTime)
{
}

void BulletStartController::OnTriggerEnter(SeedCore::Actor other)
{
    SeedCore::Actor hitActor = other;

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    isTouch = true;
}

void BulletStartController::OnTriggerExit(SeedCore::Actor other)
{
    SeedCore::Actor hitActor = other;

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    isTouch = false;
}
