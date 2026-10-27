#include"BombExplosionController.h"
#include<SeedCore/ScMath.h>

void BombExplosionController::OnStart()
{
    
}

void BombExplosionController::OnTick(float elapsedTime)
{
    switch (state)
    {
    case BombExplosionController::State::SCALE_UP:
        //爆発拡大
        sizeRate += elapsedTime / scaleUpTime;
        currentSize = SeedCore::Easing::OutSine(0.0f, targetSize, std::min(sizeRate,1.0f));
        if (sizeRate >= 1.0f)
        {
            sizeRate = 0.0f;
            state = State::WAIT;
        }
        break;
    case BombExplosionController::State::WAIT:
        //爆発待機
        waitTime -= elapsedTime;
        if (waitTime <= 0.0f)state = State::SCALE_DOWN;
        break;
    case BombExplosionController::State::SCALE_DOWN:
        //爆発縮小
        sizeRate += elapsedTime / scaleDownTime;
        currentSize = SeedCore::Easing::InSine(targetSize, 0.0f, std::min(sizeRate,1.0f));
        if (sizeRate >= 1.0f)GetWorld().DestroyActor(GetActor());
        break;
    default:
        break;
    }
    
    scale->x_ = currentSize;
    scale->y_ = currentSize;
    scale->z_ = currentSize;
}

void BombExplosionController::SetParam(const SeedCore::Vector3& pos, float targetSize)
{
    //位置をセット
    this->position = GetActor().GetComponent<SeedCore::Position>();
    this->position->x_ = pos.x;
    this->position->y_ = pos.y;
    this->position->z_ = pos.z;

    scale = GetActor().GetComponent<SeedCore::Scale>();

    //目標サイズをセット
    this->targetSize = targetSize;
}
