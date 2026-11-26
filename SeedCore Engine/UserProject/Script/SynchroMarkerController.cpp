#include "UserProject/Script/SynchroMarkerController.h"

void SynchroMarkerController::OnStart()
{
    followActor = GetWorld().GetActor(followActorID);

    if (!followActor)return;

    beforeFollowActorPosition = followActor.WorldMatrix().Translation();
}

void SynchroMarkerController::OnTick(float elapsedTime)
{
    if (!followActor)return;

    //追従オブジェクトに追従
    SeedCore::Vector3 followActorPosition = followActor.WorldMatrix().Translation();
    SeedCore::Vector3 moveVec = followActorPosition - beforeFollowActorPosition;
    SeedCore::Position* position = GetActor().GetComponent<SeedCore::Position>();
    position->x_ += moveVec.x;
    position->y_ += moveVec.y;
    position->z_ += moveVec.z;
    beforeFollowActorPosition = followActorPosition;
}
