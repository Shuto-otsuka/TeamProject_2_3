#include "UserProject/Script/BulletController.h"
#include"UserProject/Script/PlayerController.h"
#include<SeedCore/ScLog.h>

void BulletController::OnStart()
{
    playerController = GetWorld().GetActor("Player").GetComponent<PlayerController>();

    //速力と回転力を加える
    //OnStartがSetParamより後で呼ばれるためここで処理
    rigidbody->AddImpulse(moveDirection * speed);
    rigidbody->AddTorque(turnDirection * turnSpeed);
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    UpdateRemove(elapsedTime);
    //生存時間が終了したら削除
    UpdateAliveTime(elapsedTime);
}

void BulletController::SetParam(const SeedCore::Vector3& startPosition,const SeedCore::Vector3& moveDirection, float chargeRate,int cost)
{
    //発射されたタイミングでパラメータをセット

    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    scale = world.GetComponent<SeedCore::Scale>(entity);
    position = world.GetComponent<SeedCore::Position>(entity);
    rotation = world.GetComponent<SeedCore::Rotation>(entity);
    rigidbody = world.GetComponent<SeedCore::Rigidbody>(entity);
    velocity = world.GetComponent<SeedCore::Velocity>(entity);

    //初期位置をセット
    this->startPosition = startPosition;
    position->x_ = startPosition.x;
    position->y_ = startPosition.y;
    position->z_ = startPosition.z;

    //進行方向をセット
    this->moveDirection = moveDirection;
    //回転方向をセット
    turnDirection.Normalize();
 
    //チャージ率からサイズ、スピード、生存時間を計算
    size = SeedCore::Lerp(minSize, maxSize, chargeRate);//チャージされているほど大きい
    speed = SeedCore::Lerp(maxSpeed, minSpeed, chargeRate);//チャージされているほど遅い
    aliveTime = SeedCore::Lerp(minAliveTime, maxAliveTime, chargeRate);//チャージされているほど生存時間が長い
    turnSpeed = SeedCore::Lerp(minTurnSpeed, maxTurnSpeed, chargeRate);//チャージされているほどほど速い

    //スケールをセット
    scale->x_ = size;
    scale->y_ = size;
    scale->z_ = size;

    //コストをセット
    this->cost = cost;
}

void BulletController::OnCollisionEnter(SeedCore::Entity entity)
{
    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);

    //止められるオブジェクトじゃなければ終了
    if (!hitActor.HasTag("CanStop"))return;

    if (hitActor.HasTag("Bullet"))
    {
        BulletController* hitBulletController = GetWorld().GetComponent<BulletController>(entity);
        //自分の方が長く生きてたら終了
        //後で撃ったやつが先撃ってたやつに対して効果を発動するため
        if (aliveTimer > hitBulletController->GetAliveTimer())return;

        float stopTime = aliveTime - aliveTimer;//当たった弾の停止時間を残りの生存時間とする
        hitBulletController->Stop();//停止
    }
    else
    {
        //ギミック停止処理
    }
}

void BulletController::Stop()
{
    //停止処理
    isStop = true;

    //キネマティックにする
    rigidbody->bodyType_ = SeedCore::Rigidbody::BodyType::Kinematic;
}

void BulletController::UpdateRemove(float elapsedTime)
{
    //移動・回転処理
    if (isStop)return;

    if ((SeedCore::Transform::Vector(*position) - startPosition).Length() >= removeDistance)
    {
        //一定距離飛んだら消す
        Remove();
    }
}

void BulletController::UpdateAliveTime(float elapsedTime)
{
    aliveTimer += elapsedTime;
    if (aliveTimer >= aliveTime)
    {
        //生存時間が終了したら削除
        Remove();
    }
}

void BulletController::Remove()
{
    playerController->SubCostGauge(cost);//自分が圧迫してた分のコストを戻す
    GetWorld().DestroyActor(GetActor());//削除
}