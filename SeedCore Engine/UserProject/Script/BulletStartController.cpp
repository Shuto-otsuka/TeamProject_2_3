#include "UserProject/Script/BulletStartController.h"

void BulletStartController::OnStart()
{
}

void BulletStartController::OnTick(float elapsedTime)
{
}

void BulletStartController::OnTriggerEnter(SeedCore::Entity entity)
{
    isTouch = true;
}

void BulletStartController::OnTriggerExit(SeedCore::Entity entity)
{
    isTouch = false;
}
