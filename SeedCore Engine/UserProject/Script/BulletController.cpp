#include "UserProject/Script/BulletController.h"
#include"UserProject/Script/PlayerController.h"
#include"UserProject/Script/StopController.h"
#include<SeedCore/ScLog.h>

void BulletController::OnStart()
{
    player = GetWorld().GetActor("Player");
    SeedCore::Entity entity = GetActor().GetEntity();
    SeedCore::Entity playerEntity = player.GetEntity();
    playerController = GetWorld().GetComponent<PlayerController>(playerEntity);
    linearVelocity = GetWorld().GetComponent<SeedCore::LinearVelocity>(entity);
    angularVelocity = GetWorld().GetComponent<SeedCore::AngularVelocity>(entity);
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
    //通常時じゃなければ終了
    if (state != State::NORMAL)return;

    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);
    SeedCore::Name* hitActorName = GetWorld().GetComponent<SeedCore::Name>(entity);

    if (hitActor.HasTag("GravityArea"))
    {
        //重力エリアならフラグONにして終了
        isGravityArea = true;
        return;
    }

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    if (!hitActor.HasTag("CanStop"))
    {
        //止められるオブジェクトじゃない
        SC_LOG_NOTICE("止められないやつに当たった");

        if (isGravityArea)
        {
            //重力エリアであればRigidbodyをDynamicにして停止
            rigidbody->bodyType_ = SeedCore::Rigidbody::BodyType::Dynamic;
            Stop();
        }
        else
        {
            //重力エリアじゃなければ消して終了
            Remove();
        }
        return;
    }

    if (hitActor.HasTag("Bullet"))
    {
        //弾に当たった場合
        SC_LOG_NOTICE("弾に当たった");

        BulletController* hitBulletController = GetWorld().GetComponent<BulletController>(entity);
        //相手の弾のステートによって処理を変える
        switch (hitBulletController->GetState())
        {
        case State::NORMAL:
        {
            //自分の方が長く生きてたら終了
            //後で撃ったやつが先撃ってたやつに対して効果を発動するため
            if (aliveTimer > hitBulletController->GetAliveTimer())return;

            hitBulletController->Stop();//停止
            Remove();//自分は削除
            break;
        }
        case State::STOP:
        {
            //既に停止中の弾に当たった

            if (isGravityArea)
            {
                //重力エリアであればRigidbodyをDynamicにして停止
                Stop();
            }
            else
            {
                //重力エリアじゃなければ消して終了
                Remove();
            }
            break;
        }
        case State::STICK:
        {
            if (isGravityArea)
            {
                //重力エリアであればRigidbodyをDynamicにして停止
                Stop();
            }
            else
            {
                //当たった弾が止めている相手のギミックを止める
                float stopTime = aliveTime - aliveTimer;
                hitBulletController->GetNowStick()->Stop(stopTime);
                //自分は動きを止めてギミックにくっついたという判定にする
                state = State::STICK;
                //止めている相手を保存
                nowStick = hitBulletController->GetNowStick();
            }
            break; 
        }
        default:
            break;
        }
    }
    else
    {
        //ギミック停止処理
        SC_LOG_NOTICE("ギミックにあたった");

        //停止を制御するコンポネント
        StopController* stopController = GetWorld().GetComponent<StopController>(entity);
        //止める時間を残りの生存時間にする
        float stopTime = aliveTime - aliveTimer;
        //止める
        stopController->Stop(stopTime);

        //自分は動きを止めてギミックにくっついたという判定にする
        state = State::STICK;
        //止めている相手を保存
        nowStick = stopController;
    }
}

void BulletController::OnTriggerExit(SeedCore::Entity entity)
{
    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = GetWorld().GetActor(entity);

    if (hitActor.HasTag("GravityArea"))
    {
        //重力エリアならフラグOFF
        isGravityArea = false;
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
   
    //停止ステートにする
    state = State::STOP;

    //重力エリアであればRigidbodyをDynamicをする
    if (isGravityArea)
        rigidbody->bodyType_ = SeedCore::Rigidbody::BodyType::Dynamic;
    
    //衝突判定ON
    rigidbody->isTrigger_ = false;

    //速度を0にする
    linearVelocity->x_ = 0.0f;
    linearVelocity->y_ = 0.0f;
    linearVelocity->z_ = 0.0f;
    //回転速度を0にする
    angularVelocity->x_ = 0.0f;
    angularVelocity->y_ = 0.0f;
    angularVelocity->z_ = 0.0f;
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
    if (state != State::NORMAL || !position || !rotation || !rigidbody)
    {
        //常に回転0
        if(rigidbody && rigidbody->bodyType_ == SeedCore::Rigidbody::BodyType::Kinematic)
            rigidbody->MoveTarget({ position->x_,position->y_,position->z_ }, SeedCore::Quaternion::Identity, elapsedTime);
        return;
    }

    //moveDirectionの方向に移動
    SeedCore::Vector3 moveTarget = SeedCore::Transform::Vector(*position) + (moveDirection * speed * elapsedTime);

    //turnAxisを軸に回転
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(turnAxis, turnSpeed * elapsedTime);
    quaternion = SeedCore::Transform::Quat(*rotation) * quaternion;

    //キネマティック剛体を動かす
    rigidbody->MoveTarget(moveTarget, quaternion, 0.02f);

    if ((SeedCore::Transform::Vector(*position) - startPosition).Length() >= playerController->GetBulletRemoveDistance())
    {
        //一定距離飛んだら消す
        Remove();
    }
}
