#include "UserProject/Script/TitleLogoController.h"

#include<SeedCore/ScInput.h>

void TitleLogoController::OnStart()
{
    position = GetActor().GetComponent<SeedCore::Position>();

    SeedCore::Input::UnlockCursor();
    SeedCore::Input::RevealCursor();
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
