#include "UserProject/Script/GunController.h"
#include"PlayerController.h"

void GunController::OnStart()
{
    player = GetWorld().GetActor("Player");
}

void GunController::OnTick(float elapsedTime)
{
    PlayerController* playerController = player.GetComponent<PlayerController>();
    SeedCore::Scale* scale = GetActor().GetComponent<SeedCore::Scale>();//アクティブで制御できないためスケールで制御

    if (playerController->HasGun() && !playerController->IsFirstPerson())
    {
        scale->x_ = 1.0f;
        scale->y_ = 1.0f;
        scale->z_ = 1.0f;
    }
    else
    {
        scale->x_ = 0.0f;
        scale->y_ = 0.0f;
        scale->z_ = 0.0f;
    }
}
