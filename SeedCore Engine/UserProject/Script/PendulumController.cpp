#include "UserProject/Script/PendulumController.h"
#include"UserProject/Script/StopController.h"
#include<SeedCore/ScLog.h>

void PendulumController::OnStart()
{
    SeedCore::Entity entity = GetActor().GetEntity();
    myStopController = GetWorld().GetComponent<StopController>(entity);
    rotation = GetWorld().GetComponent<SeedCore::Rotation>(entity);
    rigidbody = GetWorld().GetComponent<SeedCore::Rigidbody>(entity);
    position = GetWorld().GetComponent<SeedCore::Position>(entity);

    //最初のクォータニオンと最後のクォータニオンをそれぞれセット
    startQuaternion = SeedCore::Transform::Quat(*rotation);
    SeedCore::Quaternion rotateQuaternion = SeedCore::Quaternion::CreateFromAxisAngle(SeedCore::Vector3::Right, DirectX::XM_PIDIV2);//90度回転
    endQuaternion = startQuaternion * rotateQuaternion;
}

void PendulumController::OnTick(float elapsedTime)
{
    //弾によってギミック停止中であれば終了
    if (myStopController->IsStop())return;

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
    rigidbody->MoveTarget(SeedCore::Transform::Vector(*position), quaternion, elapsedTime);
}

void PendulumController::OnInspectorGUI()
{
    
}
