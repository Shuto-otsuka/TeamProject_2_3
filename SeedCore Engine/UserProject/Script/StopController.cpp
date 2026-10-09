#include "UserProject/Script/StopController.h"

void StopController::OnStart()
{
}

void StopController::OnTick(float elapsedTime)
{
    if (!isStop)return;

    stopTime -= elapsedTime;
    if (stopTime <= 0.0f)
    {
        //一定時間経つと動き出す
        Move();
    }
}

void StopController::OnInspectorGUI()
{
    ImGui::InputFloat("StopTime", &stopTime);
}

void StopController::Stop(float stopTime)
{
    isStop = true;
    //今当たった弾の方が長く残るのであれば止まる時間を更新
    this->stopTime = std::max(this->stopTime, stopTime);
}

void StopController::Move()
{
    //動かす
    isStop = false;
    stopTime = 0.0f;
}
