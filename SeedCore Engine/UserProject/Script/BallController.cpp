#include "UserProject/Script/BallController.h"

#include<SeedCore/ScLog.h>

void BallController::OnStart()
{
    myRigid = GetActor().GetComponent<SeedCore::Rigidbody>();
    myCollider = GetActor().GetComponent<SeedCore::SphereCollider>();
    velocity = GetActor().GetComponent<SeedCore::LinearVelocity>();
    startPosition = GetActor().WorldMatrix().Translation();
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
            myCollider->isTrigger_ = false;
            isRespawn = false;
            respawnTimer = 0.0f;
        }
    }
    else if (position.y <= respawnHeight)
    {
        //高さが一定以下になったら初期位置に戻す
        myRigid->bodyType_ = SeedCore::Rigidbody::BodyType::Kinematic;
        myRigid->MoveTo(startPosition, SeedCore::Quaternion::Identity, respawnDuration);
        myCollider->isTrigger_ = true;
        isRespawn = true;
    }
}
