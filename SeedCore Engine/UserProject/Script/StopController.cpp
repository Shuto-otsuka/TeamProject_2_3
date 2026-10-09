#include "UserProject/Script/StopController.h"

void StopController::OnStart()
{
}

void StopController::OnTick(float elapsedTime)
{
    if (!isStop)return;

    stopTime -= elapsedTime;
    if (stopTime <= 0.0f)//一定時間経つと動き出す
        isStop = false;
}

void StopController::Stop(float stopTime)
{
    isStop = true;
    this->stopTime += stopTime;
}
