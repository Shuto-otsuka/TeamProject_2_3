#include "UserProject/Script/BombController.h"
#include"UserProject/Script/StopController.h"
#include<SeedCore/ScDebug.h>

void BombController::OnStart()
{
    myRigid = GetActor().GetComponent<SeedCore::Rigidbody>();
    rotation = GetActor().GetComponent<SeedCore::Rotation>();
    linearVelocity = GetActor().GetComponent<SeedCore::LinearVelocity>();
    angularVelocity = GetActor().GetComponent<SeedCore::AngularVelocity>();
    stopController = GetActor().GetComponent<StopController>();

    UpdateMoveTarget();
}

void BombController::OnTick(float elapsedTime)
{
    if (stopController->IsStop())
    {
        //停止中
    }
    else
    {
        //進行度を進める
        moveRate += moveSpeed * elapsedTime;
        bool change = false;
        if (moveRate >= 1.0f)
        {
            //1を超えたら方向転換フラグをON
            change = true;
            moveRate = 1.0f;
        }

        SeedCore::Vector3 position = GetActor().WorldMatrix().Translation();

        //ターゲットポジションを算出
        DirectX::XMFLOAT3 targetPosition = SeedCore::Vector3::Lerp(targetPoints[startNum], targetPoints[endNum], moveRate);

        //進行ベクトルから回転の軸を算出し、ターゲットクォータニオンを作成
        SeedCore::Vector3 v1 = targetPosition - position;
        v1.Normalize();
        SeedCore::Vector3 v2 = SeedCore::Vector3::Up;
        SeedCore::Vector3 axis = v2.Cross(v1);
        float angle = turnSpeed * elapsedTime;
        SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(axis, angle);
        quaternion = SeedCore::Transform::Quat(*rotation) * quaternion;

        //キネマティック剛体を動かす
        myRigid->MoveTarget(targetPosition, quaternion, elapsedTime);

        if (change)
        {
            //方向転換
            moveRate = 0.0f;

            startNum++;
            if (startNum > 3)startNum = 0;
            endNum++;
            if (endNum > 3)endNum = 0;
        }
    }
}

void BombController::OnEditorTick(float elapsedTime)
{
    UpdateMoveTarget();

    //動く方向の矢印デバッグ描画
    for (int i = 0; i < 4; i++)
    {
        int target = i < 3 ? i + 1 : 0;
        GetActor().GetService<SeedCore::DebugDraw>()->Arrow(targetPoints[i], targetPoints[target], SeedCore::Color(1.0f, 0.0f, 0.0f, 0.0f), 0.2f);
    }
}

void BombController::UpdateMoveTarget()
{
    //移動する4頂点を保存
    SeedCore::Vector3 position = GetActor().WorldMatrix().Translation();

    targetPoints[0] = SeedCore::Vector3(position.x, position.y, position.z);
    if (startX)
    {
        targetPoints[1] = SeedCore::Vector3(position.x + moveX, position.y, position.z);
        targetPoints[2] = SeedCore::Vector3(position.x + moveX, position.y, position.z + moveZ);
        targetPoints[3] = SeedCore::Vector3(position.x, position.y, position.z + moveZ);
    }                                               
    else                                            
    {                                               
        targetPoints[1] = SeedCore::Vector3(position.x, position.y, position.z + moveZ);
        targetPoints[2] = SeedCore::Vector3(position.x + moveX, position.y, position.z + moveZ);
        targetPoints[3] = SeedCore::Vector3(position.x + moveX, position.y, position.z);
    }
}
