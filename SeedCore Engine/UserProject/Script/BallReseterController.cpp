#include "UserProject/Script/BallReseterController.h"
#include "UserProject/Script/BallController.h"

void BallReseterController::OnStart()
{
    //リセット対象のボールを取得
    targetBall = GetWorld().GetActor(ballActorID);
    if(targetBall)
        targetBallController = targetBall.GetComponent<BallController>();
}

void BallReseterController::OnTick(float elapsedTime)
{
    
}

//弾が当たったタイミングで弾の方から呼ばれる
void BallReseterController::Reset()
{
    if (!targetBallController)return;

    //ボール位置をリセット
    targetBallController->Respawn();
}
