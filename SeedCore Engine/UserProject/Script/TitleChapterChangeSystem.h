#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class TitleChapterChangeSystem :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_REFLECTION_FIELD_EX("最大チャプター数")
		int maxChapterNum;
	SC_REFLECTION_FIELD_EX("チャプタースクロール速度")
		float chapterScrollSpeed;

private:
	SeedCore::Actor rightButtonActor;
	SeedCore::Actor leftButtonActor;

	int chapterNum = 0;
};
REGISTER_COMPONENT(TitleChapterChangeSystem);
