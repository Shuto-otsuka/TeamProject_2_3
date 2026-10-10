#include "UserProject/Script/CostGaugeController.h"
#include"UserProject/Script/PlayerController.h"

void CostGaugeController::OnStart()
{
    costGaugeUsed = GetWorld().GetActor("CostGaugeUsed");
    costGaugeAdd = GetWorld().GetActor("CostGaugeAdd");

    SeedCore::Entity costGaugeUsedEntity = costGaugeUsed.GetEntity();
    SeedCore::Entity costGaugeAddEntity = costGaugeAdd.GetEntity();
    SeedCore::Entity playerEntity = GetWorld().GetActor("Player").GetEntity();

    costGaugeUsedImage = GetWorld().GetComponent<SeedCore::Image>(costGaugeUsedEntity);
    costGaugeAddImage = GetWorld().GetComponent<SeedCore::Image>(costGaugeAddEntity);
    costGaugeAddPosition = GetWorld().GetComponent<SeedCore::Position>(costGaugeAddEntity);
    playerController = GetWorld().GetComponent<PlayerController>(playerEntity);
    costGaugeOverActive = GetWorld().GetComponent<SeedCore::Active>(GetWorld().GetActor("CostGaugeOverBase").GetEntity());

    costGaugeUsedImage->textureSize_.x = sizeXMin;
    costGaugeAddPosition->x_ = posXMin;
    costGaugeAddImage->textureSize_.x = sizeXMin;

    usedSizeXTarget = costGaugeUsedImage->textureSize_.x;
    addPosXTarget = costGaugeAddPosition->x_;
    addSizeXTarget = costGaugeAddImage->textureSize_.x;
}

void CostGaugeController::OnTick(float elapsedTime)
{
    //プレイヤーから使った分のコストゲージを取得
    int used = playerController->GetCostGaugeUsed();
    //使った分のコストゲージからゲージのサイズに変換
    float costGaugeUsedSize = GaugeToSizeX(used);
    //サイズをターゲットにセット
    usedSizeXTarget = costGaugeUsedSize;

    //使った分のコストゲージから追加ゲージのX座標に変換
    float costGaugeAddPos = GaugeToPosX(used);
    //位置をターゲットにセット
    addPosXTarget = costGaugeAddPos;
    //プレイヤーから増加分のコストゲージを取得
    int add = playerController->GetCostGaugeAdd();
    int total = used + add;
    bool over = false;
    if (total > playerController->GetCostGaugeMax())
    {
        add -= total - playerController->GetCostGaugeMax();//限界突破対策
        over = true;//限界突破フラグをON
    }

    //増加分のコストゲージからゲージのサイズに変換
    float costGaugeAddSize = GaugeToSizeX(add);
    //サイズをターゲットにセット
    addSizeXTarget = costGaugeAddSize;

    if (over)
    {
        //限界突破したとき用の枠線
        costGaugeOverActive->active_ = true;
        //限界突破したとき用のゲージ
        costGaugeAddImage->textureID_ = costGaugeOverSprite;
    }
    else
    {
        costGaugeOverActive->active_ = false;
        costGaugeAddImage->textureID_ = costGaugeAddSprite;
    }

    MoveGauge(elapsedTime);
}

float CostGaugeController::GaugeToPosX(int gauge)
{
    //ゲージ数値からX位置に変換する関数
    int max = playerController->GetCostGaugeMax();
    float rate = static_cast<float>(gauge) / static_cast<float>(max);
    return SeedCore::Lerp(posXMin, posXMax, rate);
}

float CostGaugeController::GaugeToSizeX(int gauge)
{
    //ゲージ数値からXサイズに変換する関数
    int max = playerController->GetCostGaugeMax();
    float rate = static_cast<float>(gauge) / static_cast<float>(max);
    return SeedCore::Lerp(sizeXMin, sizeXMax, rate);
}

float CostGaugeController::LerpConstantSpeed(float value, float target, float change)
{
    //ターゲットまで一定速度で値を変化させる
    if (value < target)
    {
        value += change;
        if (value > target)value = target;
    }
    else if (value > target)
    {
        value -= change;
        if (value < target)value = target;
    }

    return value;
}

void CostGaugeController::MoveGauge(float elapsedTime)
{
    //それぞれの値を一定速度でターゲットに動かす
    costGaugeUsedImage->textureSize_.x = LerpConstantSpeed(costGaugeUsedImage->textureSize_.x, usedSizeXTarget, gaugeMoveSpeed * elapsedTime);
    costGaugeAddPosition->x_ = LerpConstantSpeed(costGaugeAddPosition->x_, addPosXTarget, gaugeMoveSpeed * elapsedTime);
    costGaugeAddImage->textureSize_.x = LerpConstantSpeed(costGaugeAddImage->textureSize_.x, addSizeXTarget, gaugeMoveSpeed * elapsedTime);
}
