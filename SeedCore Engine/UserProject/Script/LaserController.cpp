#include "UserProject/Script/LaserController.h"
#include<SeedCore/ScDebug.h>

void LaserController::OnStart()
{
}

void LaserController::OnTick(float elapsedTime)
{
    //全てのレーザーの始点のオフセット
    SeedCore::Vector3 startOffsets[9];
    startOffsets[0] = centerOffset;
    startOffsets[1] = { centerOffset.x + deltaOffset.x,centerOffset.y + deltaOffset.y,centerOffset.z};
    startOffsets[2] = { centerOffset.x - deltaOffset.x,centerOffset.y + deltaOffset.y,centerOffset.z };
    startOffsets[3] = { centerOffset.x + deltaOffset.x,centerOffset.y - deltaOffset.y,centerOffset.z };
    startOffsets[4] = { centerOffset.x - deltaOffset.x,centerOffset.y - deltaOffset.y,centerOffset.z };
    startOffsets[5] = { centerOffset.x,centerOffset.y + deltaOffset.y,centerOffset.z };
    startOffsets[6] = { centerOffset.x,centerOffset.y - deltaOffset.y,centerOffset.z };
    startOffsets[7] = { centerOffset.x + deltaOffset.x,centerOffset.y,centerOffset.z };
    startOffsets[8] = { centerOffset.x - deltaOffset.x,centerOffset.y,centerOffset.z };
    
    //レーザーの方向は前方向
    SeedCore::Vector3 direction = GetActor().WorldMatrix().Forward();
    SeedCore::Vector3 startPositions[9];
    for (int i = 0; i < 9; i++)
    {
        float len = length;
        //レーザーの始点のオフセットをワールド位置に変換
        startPositions[i] = SeedCore::Vector3::Transform(startOffsets[i], GetActor().WorldMatrix());

        //レイキャストをして弾に当たった場合そこをレーザーの終点とする
        SeedCore::RaycastHit hit;
        if (GetActor().GetPhysics().Raycast(startPositions[i], direction, length, hit))
        {
            SeedCore::Actor hitActor = GetWorld().GetActor(hit.entityID_);
            if (hitActor.HasTag("Bullet"))
            {
                //シリンダーの長さを始点からレイがヒットした場所までの長さにする
                SeedCore::Vector3 startToHit = hit.position_ - startPositions[i];
                len = startToHit.Length();
            }
        }

        //レーザーの中心位置を算出
        SeedCore::Vector3 center = startPositions[i] + (direction * (len / 2));
        //レーザーのの回転を算出
        SeedCore::Quaternion quaternion = SeedCore::Quaternion::LookRotation(GetActor().WorldMatrix().Down(), GetActor().WorldMatrix().Up());
        //シリンダーを描画
        SeedCore::RenderInstance& renderInstance = GetWorld().GetRenderInstance();
        SeedCore::DebugDraw debugDraw(renderInstance);
        debugDraw.Cylinder(center, quaternion, tickness, len, { 1.0f,0.0f,0.0f,1.0f });
    }
}

void LaserController::OnEditorTick(float elapsedTime)
{
   
}
