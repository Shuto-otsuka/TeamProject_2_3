#include "UserProject/Script/PendulumController.h"
#include"UserProject/Script/StopController.h"
#include<SeedCore/ScLog.h>

void PendulumController::OnStart()
{
    myStopController = GetActor().GetComponent<StopController>();
    rotation = GetActor().GetComponent<SeedCore::Rotation>();
    rigidbody = GetActor().GetComponent<SeedCore::Rigidbody>();
    position = GetActor().GetComponent<SeedCore::Position>();
    angularVelocity = GetActor().GetComponent<SeedCore::AngularVelocity>();
    linearVelocity = GetActor().GetComponent<SeedCore::LinearVelocity>();

    startPosition = SeedCore::Transform::Vector(*position);

    //最初のクォータニオンと最後のクォータニオンをそれぞれセット
    startQuaternion = SeedCore::Transform::Quat(*rotation);
    SeedCore::Vector3 localAxis = SeedCore::Vector3::Right;
    SeedCore::Matrix worldMat = SeedCore::Matrix::CreateFromQuaternion(SeedCore::Transform::Quat(*rotation));
    SeedCore::Vector3 worldAxis = SeedCore::Vector3::Transform(localAxis, worldMat);//ローカル軸をワールド軸に変換
    SeedCore::Quaternion rotateQuaternion = SeedCore::Quaternion::CreateFromAxisAngle(worldAxis, DirectX::XM_PIDIV2);//90度回転
    endQuaternion = startQuaternion * rotateQuaternion;
}

void PendulumController::OnTick(float elapsedTime)
{
    //弾によってギミック停止中であれば終了
    if (myStopController->IsStop())
    {
        //速度を0にする
        linearVelocity->x_ = 0.0f;
        linearVelocity->y_ = 0.0f;
        linearVelocity->z_ = 0.0f;

        //回転速度を0にする
        angularVelocity->x_ = 0.0f;
        angularVelocity->y_ = 0.0f;
        angularVelocity->z_ = 0.0f;
        return;
    }

    //回転率を計算
    if (isFowardRotate)
    {
        rotateRate += rotateSpeed * elapsedTime;
        if (rotateRate >= 1.0f)
        {
            rotateRate = 1.0f;
            isFowardRotate = false;
        }
    }
    else
    {
        rotateRate -= rotateSpeed * elapsedTime;
        if (rotateRate <= 0.0f)
        {
            rotateRate = 0.0f;
            isFowardRotate = true;
        }
    }

    //回転率を元にSlerpでクォータニオンを作成
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::Slerp(startQuaternion, endQuaternion, rotateRate);
    //キネマティック剛体を動かす
    rigidbody->MoveTarget(startPosition, quaternion, elapsedTime);
}

void PendulumController::OnInspectorGUI()
{
    
}
