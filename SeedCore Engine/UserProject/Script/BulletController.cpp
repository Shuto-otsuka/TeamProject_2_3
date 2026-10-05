#include "UserProject/Script/BulletController.h"

void BulletController::OnStart()
{
    
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    Move(elapsedTime);
    //回転処理
    Turn(elapsedTime);
    //生存時間が終了したら削除
    UpdateAliveTime(elapsedTime);
}

void BulletController::SetParam(SeedCore::Vector3 startPosition,SeedCore::Vector3 moveDirection, float chargeRate)
{
    //発射されたタイミングでパラメータをセット

    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    scale = world.GetComponent<SeedCore::Scale>(entity);
    position = world.GetComponent<SeedCore::Position>(entity);
    rotation = world.GetComponent<SeedCore::Rotation>(entity);

    //初期位置をセット
    SeedCore::Transform::Vector(*position, startPosition);

    //進行方向をセット
    this->moveDirection = moveDirection;

    //チャージ率からサイズ、スピード、生存時間を計算
    size = SeedCore::Lerp(minSize, maxSize, chargeRate);//チャージされているほど大きい
    speed = SeedCore::Lerp(maxSpeed, minSpeed, chargeRate);//チャージされているほど遅い
    aliveTime = SeedCore::Lerp(minAliveTime, maxAliveTime, chargeRate);//チャージされているほど生存時間が長い

    //スケールをセット
    scale->x_ = size;
    scale->y_ = size;
    scale->z_ = size;
}

void BulletController::Move(float elapsedTime)
{
    //移動処理
    SeedCore::Vector3 pos = SeedCore::Transform::Vector(*position);
    pos += moveDirection * speed * elapsedTime;
    SeedCore::Transform::Vector(*position, pos);
}

void BulletController::Turn(float elapsedTime)
{
    turnAxis.Normalize();
    //回転分のクォータニオンを作成
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(turnAxis, turnSpeed * elapsedTime);
    //現在のクォータニオンに合成
    quaternion = quaternion * SeedCore::Transform::Quat(*rotation);
    //クォータニオンを適応
    SeedCore::Transform::Quat(*rotation, quaternion);
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
