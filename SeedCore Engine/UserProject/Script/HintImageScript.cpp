#include "UserProject/Script/HintImageScript.h"

void HintImageScript::OnStart()
{
    myImage = GetActor().GetComponent<SeedCore::Image>();
}

void HintImageScript::OnTick(float elapsedTime)
{
    if (!isDisplay)return;
    timer += elapsedTime;
    if (timer > fadeWaitTime)
    {
        float rate = (timer - fadeWaitTime) / fadeTime;
        if (rate >= 1.0f)
        {
            rate = 1.0f;
            isDisplay = false;
        }
        myImage->color_.a = 1.0f - rate;
    }
}

void HintImageScript::Display(float fadeTime,float fadeWaitTime)
{
    myImage->color_.a = 1.0f;
    isDisplay = true;
    timer = 0.0f;
    this->fadeTime = fadeTime;
    this->fadeWaitTime = fadeWaitTime;
}
