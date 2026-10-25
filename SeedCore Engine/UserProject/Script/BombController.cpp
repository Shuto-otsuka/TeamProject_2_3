#include "UserProject/Script/BombController.h"
#include"UserProject/Script/StopController.h"
#include"UserProject/Script/PlayerController.h"
#include<SeedCore/ScDebug.h>
#include<SeedCore/ScPrefab.h>
#include<SeedCore/ScScreen.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <format>

void BombController::OnStart()
{
    myRigid = GetActor().GetComponent<SeedCore::Rigidbody>();
    rotation = GetActor().GetComponent<SeedCore::Rotation>();
    linearVelocity = GetActor().GetComponent<SeedCore::LinearVelocity>();
    angularVelocity = GetActor().GetComponent<SeedCore::AngularVelocity>();
    stopController = GetActor().GetComponent<StopController>();

    UpdateMoveTarget();

    bombTimer = SeedCore::Prefab::Spawn("BombTimerText.prefab");
    bombTimerText = bombTimer.GetComponent<SeedCore::Text>();
    bombTimerPosition = bombTimer.GetComponent<SeedCore::Position>();
}

void BombController::OnTick(float elapsedTime)
{
    if (stopController->IsStop())
    {
        //停止中
        linearVelocity->x_ = 0.0f;
        linearVelocity->y_ = 0.0f;
        linearVelocity->z_ = 0.0f;
        angularVelocity->x_ = 0.0f;
        angularVelocity->y_ = 0.0f;
        angularVelocity->z_ = 0.0f;

        //停止中は爆弾のタイマーを非表示にする
        bombTimerText->text_ = "";
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

        explosionTimer += elapsedTime;
        if (explosionTimer >= explosionInterval)
        {
            explosionTimer = 0.0f;

            //爆発
            SeedCore::Physics* physics = GetActor().GetService<SeedCore::Physics>();
            SeedCore::DynamicArray<SeedCore::EntityID> entityIDs;

            //爆発範囲内のアクターを取得
            SeedCore::Vector3 position = GetActor().WorldMatrix().Translation();
            entityIDs = physics->Overlap(SeedCore::OverlapShapeKind::Sphere, SeedCore::Vector3(explosionRadius, 0.0f, 0.0f), position, SeedCore::Quaternion::Identity);

            for (auto& entityID : entityIDs)
            {
                SeedCore::Actor hitActor = GetWorld().GetActor(entityID);
                if (hitActor.HasTag("Player"))
                {
                    //Playerタグを持っていたら
                    PlayerController* playerController = hitActor.GetComponent<PlayerController>();
                    if (!playerController)continue;

                    //プレイヤーが爆発に巻き込まれたためリセット
                    playerController->Reset();
                }
                else if (hitActor.HasTag("Breakable"))
                {
                    //壊れるものだったら削除
                    GetWorld().DestroyActor(hitActor);
                }
            }
        }

        //爆発までの残り時間
        float timer = explosionInterval - explosionTimer;
        //テキスト反映
        std::string timerStr = std::format("{:.1f}", timer); //小数点第一以下を切り捨て
        bombTimerText->text_ = SeedCore::String(timerStr);

        //タイマーのスクリーン座標を算出し、セット
        SeedCore::Vector3 timerWorldPosition = GetActor().WorldMatrix().Translation() + timerOffset;
        SeedCore::Vector2 timerScreenPosition = SeedCore::ScreenSpace::WorldToScreen(timerWorldPosition);
        bombTimerPosition->x_ = timerScreenPosition.x;
        bombTimerPosition->y_ = timerScreenPosition.y;

        //爆発範囲のデバッグ描画
        GetActor().GetService<SeedCore::DebugDraw>()->Sphere(GetActor().WorldMatrix().Translation(), explosionRadius, SeedCore::Color(1.0f, 0.0f, 0.0f, 0.5f));
    }
}

void BombController::OnEditorTick(float elapsedTime)
{
    UpdateMoveTarget();

    //動く方向の矢印デバッグ描画
    for (int i = 0; i < 4; i++)
    {
        int target = i < 3 ? i + 1 : 0;
        GetActor().GetService<SeedCore::DebugDraw>()->Arrow(targetPoints[i], targetPoints[target], SeedCore::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.2f);
    }

    //爆発範囲のデバッグ描画
    GetActor().GetService<SeedCore::DebugDraw>()->Sphere(GetActor().WorldMatrix().Translation(), explosionRadius, SeedCore::Color(1.0f, 0.0f, 0.0f, 0.5f));
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
