#include "UserProject/Script/StageSelectButtonController.h"
#include"UserProject/Script/Button.h"

#include<SeedCore/ScScene.h>

void StageSelectButtonController::OnStart()
{
    SeedCore::Entity entity = GetActor().GetEntity();
    myButton = GetWorld().GetComponent<Button>(entity);
    myImage = GetWorld().GetComponent<SeedCore::Image>(entity);
    scale = GetWorld().GetComponent<SeedCore::Scale>(entity);

    defaultColor = myImage->color_;
    targetColor = defaultColor;

    defaultSize = scale->x_;
    targetSize = defaultSize;
}

void StageSelectButtonController::OnTick(float elapsedTime)
{
    if (isClicked)
    {
        //クリックされたときターゲットの色を変える
        targetColor = clickedColor;
        //クリックされたときターゲットのサイズを変える
        targetSize = clickedSize;

        sceneChangeWaitTime -= elapsedTime;
        if (sceneChangeWaitTime <= 0.0f)
        {
            //クリックされてから一定時間で指定のステージに遷移

            //ステージを文字列に変換
            //3とかの場合03にする
            std::string stageNumStr = std::to_string(stageNumber);
            if (stageNumStr.length() <= 1)stageNumStr = "0" + stageNumStr;
            std::string stageName = "Stage" + stageNumStr + ".scene";

            //シーン遷移
            SeedCore::Scene::Change(stageName);
        }

    }
    else if (myButton->IsOnCursor())
    {
        //カーソルが乗ってるときターゲットの色を変える
        targetColor = onCursorColor;
        //カーソルが乗ってるときターゲットのサイズを変える
        targetSize = onCursorSize;
    }
    else
    {
        targetColor = defaultColor;
        targetSize = defaultSize;
    }

    //色とサイズをターゲットに向けてLerp
    myImage->color_ = SeedCore::Color::Lerp(myImage->color_, targetColor, colorChangeSpeed * elapsedTime);
    scale->x_ = SeedCore::Lerp(scale->x_, targetSize, sizeChangeSpeed * elapsedTime);
    scale->y_ = SeedCore::Lerp(scale->y_, targetSize, sizeChangeSpeed * elapsedTime);

    if (myButton->IsClick())
    {
        //遷移待ちフラグをオンにする
        isClicked = true;

        //ターゲットに向かうスピードを2倍にする
        colorChangeSpeed *= 2.0f;
        sizeChangeSpeed *= 2.0f;
    }
}
