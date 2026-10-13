#include "UserProject/Script/Button.h"

#include<SeedCore/ScInput.h>

void Button::OnStart()
{
    SeedCore::Entity entity = GetActor().GetEntity();
    myImage = GetWorld().GetComponent<SeedCore::Image>(entity);
    position = GetWorld().GetComponent<SeedCore::Position>(entity);
    scale = GetWorld().GetComponent<SeedCore::Scale>(entity);

    //ポジション、スケール、イメージテクスチャからボタンの当たり判定の範囲を取得
    minOffset = { position->x_ - myImage->pivot_.x,position->y_ - myImage->pivot_.y };
    maxOffset = { minOffset.x + myImage->textureSize_.x * scale->x_,minOffset.y + myImage->textureSize_.y * scale->y_ };
}

void Button::OnTick(float elapsedTime)
{
    SeedCore::Vector2 cursorPosition = SeedCore::Input::MousePoint();
    if (cursorPosition.x < minOffset.x || cursorPosition.x > maxOffset.x || cursorPosition.y < minOffset.y || cursorPosition.y > maxOffset.y)
    {
        //カーソルがボタンの範囲外ならフラグオフにして終了
        isOnCursor = false;
        isClick = false;
        return;
    }

    //カーソルがボタンに乗ってる
    isOnCursor = true;

    //マウスが押された瞬間かどうかの判定
    isClick = SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Left, SeedCore::Input::OnPressed);
}

void Button::OnInspectorGUI()
{
    ImGui::Checkbox("isOnCursor", &isOnCursor);
    ImGui::Checkbox("isClick", &isClick);
}
