#include "UserProject/Script/BulletController.h"
#include"UserProject/Script/PlayerController.h"
#include<SeedCore/ScLog.h>

void BulletController::OnStart()
{
    player = GetWorld().GetActor("Player");
    SeedCore::Entity entity = GetActor().GetEntity();
    SeedCore::Entity playerEntity = player.GetEntity();
    playerController = GetWorld().GetComponent<PlayerController>(playerEntity);
    velocity = GetWorld().GetComponent<SeedCore::Velocity>(entity);
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    Move(elapsedTime);
    //生存時間が終了したら削除
    UpdateAliveTime(elapsedTime);
}

void BulletController::OnTriggerEnter(SeedCore::Entity entity)
{
    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);
    SeedCore::Name* hitActorName = GetWorld().GetComponent<SeedCore::Name>(entity);

    //プレイヤーなら終了
    if (hitActor == player)return;

    //止められるオブジェクトじゃなければ消して終了
    if (!hitActor.HasTag("CanStop"))
    {
        Remove();
        SC_LOG_NOTICE("止められないやつに当たった");
        return;
    }

    if (hitActor.HasTag("Bullet"))
    {
        //弾に当たった場合
        SC_LOG_NOTICE("弾に当たった");

        BulletController* hitBulletController = GetWorld().GetComponent<BulletController>(entity);
        //自分の方が長く生きてたら終了
        //後で撃ったやつが先撃ってたやつに対して効果を発動するため
        if (aliveTimer > hitBulletController->GetAliveTimer())return;

        hitBulletController->Stop();//停止
        Remove();//自分は削除
    }
    else
    {
        //ギミック停止処理
        SC_LOG_NOTICE("ギミックにあたった");
    }
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

    //初期位置をセット
    this->startPosition = startPosition;
    position->x_ = startPosition.x;
    position->y_ = startPosition.y;
    position->z_ = startPosition.z;

    //進行方向をセット
    this->moveDirection = moveDirection;
    //回転方向をセット
    turnAxis.Normalize();
 
    //チャージ率からサイズ、スピード、生存時間を計算
    size = SeedCore::Lerp(minSize, maxSize, chargeRate);//チャージされているほど大きい
    speed = SeedCore::Lerp(maxSpeed, minSpeed, chargeRate);//チャージされているほど遅い
    aliveTime = SeedCore::Lerp(minAliveTime, maxAliveTime, chargeRate);//チャージされているほど生存時間が長い

    //スケールをセット
    scale->x_ = size;
    scale->y_ = size;
    scale->z_ = size;

    //コストをセット
    this->cost = cost;
}

void BulletController::Stop()
{
    //停止処理
    isStop = true;

    //衝突判定ON
    rigidbody->isTrigger_ = false;
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

void BulletController::Move(float elapsedTime)
{
    //移動・回転処理

    //停止中なら終了
    if (isStop || !position || !rotation || !rigidbody)
    {
        //速度を0にする
        velocity->x_ = 0.0f;
        velocity->y_ = 0.0f;
        velocity->z_ = 0.0f;

        SC_LOG_NOTICE("止まってる");
        return;
    }

    //moveDirectionの方向に移動
    SeedCore::Vector3 moveTarget = SeedCore::Transform::Vector(*position) + (moveDirection * speed * elapsedTime);

    //turnAxisを軸に回転
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(turnAxis, turnSpeed * elapsedTime);
    quaternion = SeedCore::Transform::Quat(*rotation) * quaternion;

    //キネマティック剛体を動かす
    rigidbody->MoveTarget(moveTarget, quaternion, 0.02f);

    if ((SeedCore::Transform::Vector(*position) - startPosition).Length() >= removeDistance)
    {
        //一定距離飛んだら消す
        Remove();
    }
}
