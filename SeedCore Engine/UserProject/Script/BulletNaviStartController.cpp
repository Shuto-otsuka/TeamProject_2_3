#include "UserProject/Script/BulletNaviStartController.h"

void BulletNaviStartController::OnStart()
{
}

void BulletNaviStartController::OnTick(float elapsedTime)
{

}

void BulletNaviStartController::OnTriggerExit(SeedCore::Entity entity)
{
    isTouch = false;
}

void BulletNaviStartController::OnTriggerStay(SeedCore::Entity entity)
{
    isTouch = true;
}
