#include "UserProject/Script/BulletNaviController.h"
#include"UserProject/Script/PlayerController.h"

#include<SeedCore/ScPrefab.h>

void BulletNaviController::OnStart()
{
    bulletStart = GetWorld().GetActor("BulletStart");
    bulletEnd = GetWorld().GetActor("BulletEnd");
    line = GetWorld().GetActor("Line");

    SeedCore::Entity bulletStartEntity = bulletStart.GetEntity();
    SeedCore::Entity bulletEndEntity = bulletEnd.GetEntity();
    SeedCore::Entity playerEntity = GetWorld().GetActor("Player").GetEntity();

    bulletStartPosition = GetWorld().GetComponent<SeedCore::Position>(bulletStartEntity);
    bulletEndPosition = GetWorld().GetComponent<SeedCore::Position>(bulletEndEntity);
    bulletStartScale = GetWorld().GetComponent<SeedCore::Scale>(bulletStartEntity);
    bulletEndScale = GetWorld().GetComponent<SeedCore::Scale>(bulletEndEntity);
    bulletStartActive = GetWorld().GetComponent<SeedCore::Active>(bulletStartEntity);
    bulletEndActive = GetWorld().GetComponent<SeedCore::Active>(bulletEndEntity);
    playerController = GetWorld().GetComponent<PlayerController>(playerEntity);
}

void BulletNaviController::OnTick(float elapsedTime)
{
    float chargeRate = playerController->GetChargeRate();

    if (chargeRate <= 0.0f)
    {
        //チャージ中じゃなければナビを全て削除
        bulletStartActive->active_ = false;
        bulletEndActive->active_ = false;

        for (auto& cube : line.Children())
        {
            SeedCore::Entity cubeEntity = cube.GetEntity();
            SeedCore::Active* cubeActive = GetWorld().GetComponent<SeedCore::Active>(cubeEntity);
            cubeActive->active_ = false;
        }

        //点線の初期位置の変化をリセット
        cubeStartInterval = 0.0f;
    }
    else
    {
        //チャージ中にナビをアクティブ
        bulletStartActive->active_ = true;
        bulletEndActive->active_ = true;

        //弾の発射位置とターゲット位置を取得
        SeedCore::Vector3 sP = playerController->GetBulletPosition();
        SeedCore::Vector3 eP = playerController->GetBulletTargetPosition();

        //位置をアクターに設定
        bulletStartPosition->x_ = sP.x;
        bulletStartPosition->y_ = sP.y;
        bulletStartPosition->z_ = sP.z;

        bulletEndPosition->x_ = eP.x;
        bulletEndPosition->y_ = eP.y;
        bulletEndPosition->z_ = eP.z;

        //今のチャージ率からサイズを算出
        float size = SeedCore::Lerp(minSize, maxSize, chargeRate);

        //サイズをセット
        bulletStartScale->x_ = size;
        bulletStartScale->y_ = size;
        bulletStartScale->z_ = size;
        bulletEndScale->x_ = size;
        bulletEndScale->y_ = size;
        bulletEndScale->z_ = size;

        //点線のスタート位置を移動(スクロール)
        cubeStartInterval += scrollSpeed * elapsedTime;
        if (cubeStartInterval >= interval)
            cubeStartInterval = 0.0f;

        //いったんキューブを全て非表示
        for (auto& cube : line.Children())
        {
            SeedCore::Entity cubeEntity = cube.GetEntity();
            SeedCore::Active* cubeActive = GetWorld().GetComponent<SeedCore::Active>(cubeEntity);
            cubeActive->active_ = false;
        }

        //始点から終点までのベクトルを算出
        SeedCore::Vector3 direction = eP - sP;
        direction.Normalize();

        //点線の初期位置をセット
        SeedCore::Vector3 point = sP + (direction * cubeStartInterval);
  
        for(auto& cube : line.Children())
        {
            SeedCore::Entity cubeEntity = cube.GetEntity();
            SeedCore::Active* cubeActive = GetWorld().GetComponent<SeedCore::Active>(cubeEntity);

            //pointを間隔空けて更新
            point += direction * interval;

            //pointが終点を過ぎている場合終了
            if ((point - sP).Length() > (eP - sP).Length())
            {
                cubeActive->active_ = false;
                continue;
            }

            //point位置にキューブを配置
            cubeActive->active_ = true;
            SeedCore::Position* position = GetWorld().GetComponent<SeedCore::Position>(cubeEntity);
            position->x_ = point.x;
            position->y_ = point.y;
            position->z_ = point.z;
        }
    }
}
