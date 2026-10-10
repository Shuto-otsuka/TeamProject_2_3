#include "UserProject/Script/ChargeGaugeController.h"
#include"UserProject/Script/PlayerController.h"

void ChargeGaugeController::OnStart()
{
    chargeGaugeNow = GetWorld().GetActor("ChargeGaugeNow");
    chargeGaugeKnob = GetWorld().GetActor("ChargeGaugeKnob");
    chargeGaugeBase = GetWorld().GetActor("ChargeGaugeBase");

    SeedCore::Entity chargeGaugeNowEntity = chargeGaugeNow.GetEntity();
    SeedCore::Entity chargeGaugeKnobEntity = chargeGaugeKnob.GetEntity();
    SeedCore::Entity chargeGaugeBaseEntity = chargeGaugeBase.GetEntity();
    SeedCore::Entity playerEntity = GetWorld().GetActor("Player").GetEntity();

    chargeGaugeNowImage = GetWorld().GetComponent<SeedCore::Image>(chargeGaugeNowEntity);
    chargeGaugeKnobPosition = GetWorld().GetComponent<SeedCore::Position>(chargeGaugeKnobEntity);
    chargeGaugeBaseActive = GetWorld().GetComponent<SeedCore::Active>(chargeGaugeBaseEntity);
    chargeGaugeKnobActive = GetWorld().GetComponent<SeedCore::Active>(chargeGaugeKnobEntity);
    chargeGaugeNowActive = GetWorld().GetComponent<SeedCore::Active>(chargeGaugeNowEntity);
    playerController = GetWorld().GetComponent<PlayerController>(playerEntity);
}

void ChargeGaugeController::OnTick(float elapsedTime)
{
    //プレイヤーからチャージ率を取得
    float rate = playerController->GetChargeRate();
    if (rate <= 0.0f)
    {
        //チャージしていないときは全て非表示
        chargeGaugeBaseActive->active_ = false;
        chargeGaugeNowActive->active_ = false;
        chargeGaugeKnobActive->active_ = false;
    }
    else
    {
        chargeGaugeBaseActive->active_ = true;
        chargeGaugeNowActive->active_ = true;
        chargeGaugeKnobActive->active_ = true;

        //チャージ率からゲージのサイズを決める
        chargeGaugeNowImage->textureSize_.x = SeedCore::Lerp(sizeXMin, sizeXMax, rate);
        //チャージ率からノブの位置を決める
        chargeGaugeKnobPosition->x_ = SeedCore::Lerp(posXMin, posXMax, rate);
    }
}
