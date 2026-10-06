#include "UserProject/Script/BulletController.h"
#include"UserProject/Script/PlayerController.h"

void BulletController::OnStart()
{
    playerController = GetWorld().GetActor("Player").GetComponent<PlayerController>();
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    Move(elapsedTime);
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

    //初期位置をセット
    this->startPosition = startPosition;
    position->x_ = startPosition.x;
    position->y_ = startPosition.y;
    position->z_ = startPosition.z;

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

    //コストをセット
    this->cost = cost;
}

void BulletController::OnCollisionEnter(SeedCore::Entity entity)
{
    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);

    //止められるオブジェクトじゃなければ終了
    if (!hitActor.HasTag("CanStop"))return;

}

void BulletController::Move(float elapsedTime)
{
    //移動・回転処理

    SeedCore::Vector3 pos = SeedCore::Transform::Vector(*position);
    //moveDirectionの方向に移動
    pos += moveDirection * speed * elapsedTime;

    turnAxis.Normalize();
    //回転分のクォータニオンを作成
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(turnAxis, turnSpeed * elapsedTime);
    //現在のクォータニオンに合成
    quaternion = quaternion * SeedCore::Transform::Quat(*rotation);
  
    //キネマティック剛体を動かす
    rigidbody->MoveTarget(pos, quaternion, elapsedTime);

    if ((SeedCore::Transform::Vector(*position) - startPosition).Length() >= removeDistance)
    {
        //一定距離飛んだら消す
        Remove();
    }
}

void BulletController::UpdateAliveTime(float elapsedTime)
{
    aliveTimer -= elapsedTime;
    if (aliveTime >= aliveTime)
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
