#include "UserProject/Script/TitleChapterChangeSystem.h"
#include "Button.h"

void TitleChapterChangeSystem::OnStart()
{
    rightButtonActor = GetWorld().GetActor("Right");
    leftButtonActor = GetWorld().GetActor("Left");
}

void TitleChapterChangeSystem::OnTick(float elapsedTime)
{
    if (!rightButtonActor || !leftButtonActor)return;

    SeedCore::Image* rightButtonImage = rightButtonActor.GetComponent<SeedCore::Image>();
    SeedCore::Image* leftButtonImage = leftButtonActor.GetComponent<SeedCore::Image>();
    Button* rightButton = rightButtonActor.GetComponent<Button>();
    Button* leftButton = leftButtonActor.GetComponent<Button>();

    //現在表示しているチャプターから左右のボタンを押せるかどうか判定する
    if (chapterNum > 0)
    {
        leftButtonImage->texturePosition_.x = 0.0f;
        if (leftButton->IsClick())
            chapterNum--;
    }
    else
    {
        leftButtonImage->texturePosition_.x = 83.0f;
    }

    if (chapterNum < maxChapterNum)
    {
        rightButtonImage->texturePosition_.x = 0.0f;
        if (rightButton->IsClick())
            chapterNum++;
    }
    else
    {
        rightButtonImage->texturePosition_.x = 83.0f;
    }

    //表示チャプターの位置にLerp
    SeedCore::Vector2 buttonsTargetPosition = SeedCore::Vector2(-1920.0f * chapterNum, 0.0f);
    SeedCore::Vector2 buttonsNowPosition = GetActor().WorldMatrix().Translation().xy;
    SeedCore::Vector2 buttonsPos = SeedCore::Vector2::Lerp(buttonsNowPosition, buttonsTargetPosition, chapterScrollSpeed * elapsedTime);
    SeedCore::Position* buttonsPosition = GetActor().GetComponent<SeedCore::Position>();
    buttonsPosition->x_ = buttonsPos.x;
    buttonsPosition->y_ = buttonsPos.y;
}
