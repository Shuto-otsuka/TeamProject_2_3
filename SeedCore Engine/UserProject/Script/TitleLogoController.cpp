#include "UserProject/Script/TitleLogoController.h"

void TitleLogoController::OnStart()
{
    SeedCore::Entity entity = GetActor().GetEntity();

    position = GetWorld().GetComponent<SeedCore::Position>(entity);
}

void TitleLogoController::OnTick(float elapsedTime)
{
    //上下にsin波で移動
    rad += (elapsedTime * DirectX::XM_2PI) / moveTime;
    float rate = sinf(rad);
    rate = (rate + 1) / 2;

    float posY = SeedCore::Lerp(minPosY, maxPosY, rate);

    position->y_ = posY;
}
