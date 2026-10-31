#include "UserProject/Script/BallReseterController.h"
#include "UserProject/Script/BallController.h"

void BallReseterController::OnStart()
{
    spinActor = GetActor().Children().at(0);
    //リセット対象のボールを取得
    targetBall = GetWorld().GetActor(ballActorID);
    if(targetBall)
        targetBallController = targetBall.GetComponent<BallController>();
}

void BallReseterController::OnTick(float elapsedTime)
{
    //子オブジェクトを回転させる処理
    SeedCore::Rotation* spinRot = spinActor.GetComponent<SeedCore::Rotation>();
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(SeedCore::Vector3(0.0f, 1.0f, 0.0f), turnSpeed * elapsedTime);
    quaternion = SeedCore::Transform::Quat(*spinRot) * quaternion;
    spinRot->x_ = quaternion.x;
    spinRot->y_ = quaternion.y;
    spinRot->z_ = quaternion.z;
    spinRot->w_ = quaternion.w;
}

//弾が当たったタイミングで弾の方から呼ばれる
void BallReseterController::Reset()
{
    if (!targetBallController)return;

    //ボール位置をリセット
    targetBallController->Respawn();
}
