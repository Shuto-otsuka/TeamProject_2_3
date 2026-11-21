#include "UserProject/Script/StopController.h"
#include "PlayerController.h"

#include<SeedCore/ScPrefab.h>
#include<SeedCore/ScScreen.h>
#include <format>

void StopController::OnStart()
{
    position = GetActor().GetComponent<SeedCore::Position>();
    player = GetWorld().GetActor("Player");
    thirdPersonCameraActor = GetWorld().GetActor("CameraBrain");
    firstPersonCameraActor = GetWorld().GetActor("FirstPersonCameraBrain");
    
    playerController = player.GetComponent<PlayerController>();

    SeedCore::Actor moveAudioActor = GetWorld().GetActor("AudioMoveStart");
    if (moveAudioActor)moveAudio = moveAudioActor.GetComponent<SeedCore::AudioSource>();
}

void StopController::OnTick(float elapsedTime)
{
    if (!isStop)return;

    position = GetActor().GetComponent<SeedCore::Position>();

    if (stopTimer)
    {
        //大量に出てくるオブジェクトはアドレスが変わる可能性があるためコンポネントを取得
        stopTimerText = stopTimer.GetComponent<SeedCore::Text>();
        stopTimerPosition = stopTimer.GetComponent<SeedCore::Position>();
    }

    stopTime -= elapsedTime;
    if (stopTime <= 0.0f)
    {
        //一定時間経つと動き出す
        Move();
    }

    //タイマーがなければ以下の処理を行わない
    if (!stopTimer)return;

    SeedCore::Active* stopTimerActive = stopTimer.GetComponent<SeedCore::Active>();

    //タイマーの更新

    SeedCore::Position* cameraPosition = nullptr;
    SeedCore::Vector3 cameraFront;
    if (playerController->IsFirstPerson())
    {
        cameraPosition = firstPersonCameraActor.GetComponent<SeedCore::Position>();
        cameraFront = firstPersonCameraActor.WorldMatrix().Forward();
    }
    else
    {
        cameraPosition = thirdPersonCameraActor.GetComponent<SeedCore::Position>();
        cameraFront = thirdPersonCameraActor.WorldMatrix().Forward();
    }
    SeedCore::Vector3 cameraToBullet = SeedCore::Transform::Vector(*position) - SeedCore::Transform::Vector(*cameraPosition);
    cameraToBullet.Normalize();
    if (cameraFront.Dot(cameraToBullet) < 0.0f)
    {
        //カメラが爆弾と真反対の方向向いてたらテキストを表示しない
        stopTimerActive->active_ = false;
    }
    else
    {
        stopTimerActive->active_ = true;

        //タイマーの更新
        std::string stopTimeStr = std::format("Stop {:.1f}", stopTime);//小数点第一以下の非表示
        stopTimerText->text_ = SeedCore::String(stopTimeStr);

        //スクリーン座標変換&オフセットを適応
        SeedCore::Vector3 worldPosition;
        worldPosition.x = position->x_ + stopTimerOffset.x;
        worldPosition.y = position->y_ + stopTimerOffset.y;
        worldPosition.z = position->z_ + stopTimerOffset.z;
        SeedCore::Vector2 screenPosition = SeedCore::ScreenSpace::WorldToScreen(worldPosition);
        stopTimerPosition->x_ = screenPosition.x;
        stopTimerPosition->y_ = screenPosition.y;
    }
}

void StopController::OnInspectorGUI()
{
    ImGui::InputFloat("StopTime", &stopTime);
}

void StopController::Stop(float stopTime)
{
    if (!isStop)
    {
        //ストップタイマーを出現させる
        stopTimer = SeedCore::Prefab::Spawn(SeedCore::String("StopTimerText.prefab"));
        stopTimerText = stopTimer.GetComponent<SeedCore::Text>();
        //スクリーン座標変換&オフセットを適応
        //ただのテキスト表示なため回転を考慮しない
        stopTimerPosition = stopTimer.GetComponent<SeedCore::Position>();
        SeedCore::Vector3 worldPosition;
        worldPosition.x = position->x_ + stopTimerOffset.x;
        worldPosition.y = position->y_ + stopTimerOffset.y;
        worldPosition.z = position->z_ + stopTimerOffset.z;
        SeedCore::Vector2 screenPosition = SeedCore::ScreenSpace::WorldToScreen(worldPosition);
        stopTimerPosition->x_ = screenPosition.x;
        stopTimerPosition->y_ = screenPosition.y;
    }

    isStop = true;
    //今当たった弾の方が長く残るのであれば止まる時間を更新
    this->stopTime = std::max(this->stopTime, stopTime);

    //連動するアクターも止める
    for (UINT32 linkActorID : linkActorIDs_)
    {
        SeedCore::Actor linkActor = GetWorld().GetActor(linkActorID);
        if (!linkActor)continue;
        StopController* stopController = linkActor.GetComponent<StopController>();
        if (!stopController)continue;
        if (stopController->IsStop())continue;
        stopController->Stop(stopTime);
    }
}

void StopController::Move()
{
    if (isStop)
    {
        //ストップタイマーを削除
        GetWorld().DestroyActor(stopTimer);
        stopTimerText = nullptr;
        stopTimerPosition = nullptr;

        if (moveAudio)moveAudio->Play();
    }

    //動かす
    isStop = false;
    stopTime = 0.0f;

    for (UINT32 linkActorID : linkActorIDs_)
    {
        SeedCore::Actor linkActor = GetWorld().GetActor(linkActorID);
        if (!linkActor)continue;
        StopController* stopController = linkActor.GetComponent<StopController>();
        if (!stopController)continue;
        if (!stopController->IsStop())continue;
        stopController->Move();
    }
}
