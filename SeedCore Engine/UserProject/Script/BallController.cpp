#include "UserProject/Script/BallController.h"
#include "UserProject/Script/StopController.h"

#include<SeedCore/ScLog.h>

void BallController::OnStart()
{
    myRigid = GetActor().GetComponent<SeedCore::Rigidbody>();
    myCollider = GetActor().GetComponent<SeedCore::SphereCollider>();
    velocity = GetActor().GetComponent<SeedCore::LinearVelocity>();
    stopController = GetActor().GetComponent<StopController>();
    startPosition = GetActor().WorldMatrix().Translation();

    noStopAirResistance = myRigid->linearDrag_;
}

void BallController::OnTick(float elapsedTime)
{
    SeedCore::Vector3 position = GetActor().WorldMatrix().Translation();

    if (isRespawn)
    {
        respawnTimer += elapsedTime;
        if (respawnTimer >= respawnDuration)
        {
            velocity->x_ = 0.0f;
            velocity->y_ = 0.0f;
            velocity->z_ = 0.0f;
            myRigid->bodyType_ = SeedCore::Rigidbody::BodyType::Dynamic;
            myRigid->Dispatch();
            myCollider->isTrigger_ = false;
            isRespawn = false;
        }
    }
    else if (position.y <= respawnHeight)
    {
        //高さが一定以下になったら初期位置に戻す
        Respawn();
    }

    if (!isStop && stopController->IsStop())
    {
        //止められたタイミングで速度を0にして、重くする
        velocity->x_ = 0.0f;
        velocity->y_ = 0.0f;
        velocity->z_ = 0.0f;
        myRigid->linearDrag_ = stopAirResistance;
        myRigid->Dispatch();
        isStop = stopController->IsStop();
    }
    else if (isStop && !stopController->IsStop())
    {
        //止められてないとき軽くする
        myRigid->linearDrag_ = noStopAirResistance;
        myRigid->Dispatch();
        isStop = stopController->IsStop();
    }
}

void BallController::Respawn()
{
    //高さが一定以下になったら初期位置に戻す
    myRigid->bodyType_ = SeedCore::Rigidbody::BodyType::Kinematic;
    myRigid->MoveTo(startPosition, SeedCore::Quaternion::Identity, respawnDuration);
    myRigid->Dispatch();
    myCollider->isTrigger_ = true;
    isRespawn = true;
    respawnTimer = 0.0f;
}
