#include "UserProject/Script/BulletController.h"

void BulletController::OnStart()
{
    
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    Move(elapsedTime);
    //生存時間が終了したら削除
    UpdateAliveTime(elapsedTime);
}

void BulletController::SetParam(SeedCore::Vector3 startPosition,SeedCore::Vector3 moveDirection, float chargeTime)
{
    //発射されたタイミングでパラメータをセット

    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    scale = world.GetComponent<SeedCore::Scale>(entity);
    position = world.GetComponent<SeedCore::Position>(entity);

    //初期位置をセット
    position->x_ = startPosition.x;
    position->y_ = startPosition.y;
    position->z_ = startPosition.z;

    //進行方向をセット
    this->moveDirection = moveDirection;

    chargeTime = std::min(chargeTime, maxChargeTime);
    float rate = chargeTime / maxChargeTime;//チャージ時間からチャージ率を計算

    //チャージ率からサイズとスピードを計算
    size = SeedCore::Lerp(minSize, maxSize, rate);//チャージされているほど大きい
    speed = SeedCore::Lerp(maxSpeed, minSpeed, rate);//チャージされているほど遅い
    aliveTime = SeedCore::Lerp(minAliveTime, maxAliveTime, rate);//チャージされているほど生存時間が長い

    //スケールをセット
    scale->x_ = size;
    scale->y_ = size;
    scale->z_ = size;
}

void BulletController::Move(float elapsedTime)
{
    //移動処理
    SeedCore::Vector3 pos = position->Vector();
    pos += moveDirection * speed * elapsedTime;
    position->x_ = pos.x;
    position->y_ = pos.y;
    position->z_ = pos.z;
}

void BulletController::UpdateAliveTime(float elapsedTime)
{
    aliveTime -= elapsedTime;
    if (aliveTime <= 0.0f)
    {
        //生存時間が終了したら削除
        GetWorld().DestroyActor(GetActor());
    }
}
