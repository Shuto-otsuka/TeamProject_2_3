#include "UserProject/Script/CameraController.h"
#include <SeedCore/ScInput.h>    

void CameraController::OnStart()
{
    SeedCore::World& world = GetWorld();
    player = world.GetActor("Player");
    position = world.GetComponent<SeedCore::Position>(GetActor().GetEntity());
    rotation = world.GetComponent<SeedCore::Rotation>(GetActor().GetEntity());
    playerPosition = world.GetComponent<SeedCore::Position>(player.GetEntity());

    //注視点をプレイヤーの位置に設定
    smoothFocusPoint = playerPosition->Vector();
    //回転初期化
    Rotate();
    isCursorLock = false;
}

void CameraController::OnTick(float elapsedTime)
{
    //注視点追従処理
    SmoothFocus(elapsedTime);
    //カメラ回転処理
    Rotate();
    //フォーカス処理
    LookPlayer();
    //ズームイン、ズームアウト処理
    UpdateZoom();
    //デバッグ関連
    UpdateDebug();
}

void CameraController::SmoothFocus(float elapsedTime)
{
    //注視点をプレイヤーの位置にLerp
    SeedCore::Vector3 targetPosition = playerPosition->Vector();
    targetPosition.y += lookPlayerHeight;
    smoothFocusPoint = SeedCore::Vector3::Lerp(smoothFocusPoint, targetPosition, smoothFocusSpeed * elapsedTime);
}

void CameraController::LookPlayer()
{
    //ピッチ、ヨーから回転行列を作成
    float pitchRad = DirectX::XMConvertToRadians(pitch);
    float yawRad = DirectX::XMConvertToRadians(yaw + 180.0f);
    SeedCore::Matrix rotationMatrix = SeedCore::Matrix::CreateFromYawPitchRoll(SeedCore::Vector3(pitchRad, yawRad, 0.0f));
    //後ろ方向をかけることで注視点からのオフセットを算出
    SeedCore::Vector3 offset = SeedCore::Vector3::TransformNormal({ 0.0f,0.0f,-distance }, rotationMatrix);
    //ポジションを確定
    SeedCore::Vector3 p = smoothFocusPoint + offset;
    position->x_ = p.x;
    position->y_ = p.y;
    position->z_ = p.z;
}

void CameraController::Rotate()
{
    //右クリックでカーソル固定、解除切り替え
    if (SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Right, SeedCore::Input::OnPressed) ||
        SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Right, SeedCore::Input::OnReleased))
        ChangeCursorMode();

    //マウスカーソルが自由に動かせる状態なら回転処理しない
    if (!isCursorLock)return;

    //マウスの移動量を取得
    SeedCore::Vector2 mouseMove = SeedCore::Input::MouseMotion();
    //マウスの移動量をピッチ、ヨーに適応
    pitch += mouseMove.y * horizontalSensitivity;
    yaw += mouseMove.x * verticalSensitivity;
    //ピッチをクランプ
    pitch = std::clamp(pitch, pitchMin, pitchMax);
    //ヨーを0から360の範囲に
    while (yaw > 360)yaw -= 360;
    while (yaw < 0)yaw += 360;

    //ピッチとヨーからクォータニオンを作成
    float pitchRad = -DirectX::XMConvertToRadians(pitch);
    float yawRad = DirectX::XMConvertToRadians(yaw);
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromYawPitchRoll(SeedCore::Vector3(pitchRad, yawRad, 0.0f));
    quaternion.Normalize();
    //現在のローテーションにセット
    rotation->x_ = quaternion.x;
    rotation->y_ = quaternion.y;
    rotation->z_ = quaternion.z;
    rotation->w_ = quaternion.w;
}

void CameraController::UpdateZoom()
{
    //ホイールでズームイン、ズームアウト
    float moveWheel = SeedCore::Input::MouseWheel();
    distance -= moveWheel * zoomSpeed;
    distance = std::clamp(distance, minDistance, maxDistance);
}

void CameraController::UpdateDebug()
{
   
}

void CameraController::ChangeCursorMode()
{
    if (isCursorLock)
    {
        //カーソル固定解除、見せる
        SeedCore::Input::UnlockCursor();
        SeedCore::Input::RevealCursor();
        isCursorLock = false;
    }
    else
    {
        //カーソル中央固定、隠す
        SeedCore::Input::LockCursor(SeedCore::Vector2(960,540));
        SeedCore::Input::HideCursor();
        isCursorLock = true;
    }
}
