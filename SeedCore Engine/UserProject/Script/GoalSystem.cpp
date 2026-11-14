#include "GoalSystem.h"
#include "PlayerController.h"

#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScLog.h>
#include <SeedCore/ScMath.h>
#include <SeedCore/ScInput.h>
#include <SeedCore/ScScene.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>

using namespace SeedCore;

void GoalSystem::OnStart()
{
	isGoal_ = false;
	isDecided_ = false;
	resultTimer_ = 0.0f;

	World& world = GetWorld();

	// 後段処理でスケールを0にするのでここで一応1に戻す
	world.GetService<GameTimer>()->TimeScale(1.0f);

	// 今回はどのステージでもResultUIを親として子供にそれぞれのUIの構成にする
	// ゴールするまでは表示したくないのでActiveはfalseに
	resultActor_ = world.GetActor("ResultUI");
	resultActor_.Active(false);

	Scale* resultScale = resultActor_.GetComponent<Scale>();
	if (resultScale)
	{
		// 拡大演出のため初期値を0.05fに、親なので子も追従する
		// Pivotの調整はEditorの方で
		Transform::Vector(*resultScale, Vector3(0.05f, 0.05f, 0.05f));
	}

	// それぞれのUIのActorを取得する
	player_ = world.GetActor("Player");
	toNextSceneActor_ = world.GetActor("NextButtonUI");
	toTitleSceneActor_ = world.GetActor("TitleButtonUI");
	toRetrySceneActor_ = world.GetActor("RetryButtonUI");

	SeedCore::Actor clearAudioActor = GetWorld().GetActor("AudioClear");
	if (clearAudioActor)clearAudio = clearAudioActor.GetComponent<SeedCore::AudioSource>();
}

void GoalSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();

	if (!isGoal_)
	{
		Physics& physics = *actor.GetService<Physics>();
		Vector3 center = actor.WorldMatrix().Translation();

		// リフレクションで出す範囲を使って、近くの範囲内のEntityを検出する
		DynamicArray<EntityID> entityIDs = physics.Overlap(OverlapShapeKind::Sphere, Vector3(range_, 0.0f, 0.0f), center, Quaternion::Identity);

		for (EntityID entityID : entityIDs)
		{
			Actor other = world.GetActor(entityID);

			// 範囲内にいるActorの内、Playerタグを持つActorを探し出す
			if (other.HasTag("Player"))
			{
				isGoal_ = true;
				resultActor_.Active(true);
				ApplyResult();//ここでリザルトのテキストをセット
				if (clearAudio)clearAudio->Play();
				PlayerController* playerController = other.GetComponent<PlayerController>();
				if (playerController)
					playerController->Goal();
				break;
			}
		}
	}
	else
	{
		// リザルト時はゲームは止まってほしいのでタイムスケール0
		// これより先はタイムスケールに関係ないタイマーを使う
		world.GetService<GameTimer>()->TimeScale(0.0f);
		resultTimer_ += world.GetService<GameTimer>()->UnscaledDeltaTime();

		// 上がってちょっと超えて下がる感じの動き
		float scale = Easing::OutBack(0.05f, 1.0f, resultTimer_ / directionTime_, 2.68f);
		Scale* resultScale = resultActor_.GetComponent<Scale>();
		if (resultScale)
		{
			// スケールを上げていく
			Transform::Vector(*resultScale, Vector3(scale, scale, scale));
		}

		// 演出中なら入力を受け付けない
		if (resultTimer_ < directionTime_ || isDecided_)
		{
			return;
		}

		// Gameビュー上でカーソルの位置から矩形範囲内の全アクターのIDを返す
		DynamicArray<EntityID> hitEntityIDs = actor.GetService<Physics>()->Overlap2D(OverlapShapeKind::Rect, Vector2(0.5f, 0.5f), Input::MousePoint(), 0.0f);
		Actor hoveredButton;
		for (EntityID hitEntityID : hitEntityIDs)
		{
			Actor hitActor = world.GetActor(hitEntityID);
			// ホバーされているかを判定する
			if (hitActor == toNextSceneActor_ || hitActor == toTitleSceneActor_ || hitActor == toRetrySceneActor_)
			{
				hoveredButton = hitActor;
				break;
			}
		}

		float deltaTime = world.GetService<GameTimer>()->UnscaledDeltaTime();
		// それぞれで倍率を常に掛けるためにループを回す
		for (Actor actor : {toNextSceneActor_, toTitleSceneActor_, toRetrySceneActor_})
		{
			Scale* buttonScale = actor.GetComponent<Scale>();
			float target = (actor == hoveredButton) ? 1.1f : 1.0f;
			float scale = buttonScale->x_ + (target - buttonScale->x_) * Min(1.0f, deltaTime * 15.0f);
			Transform::Vector(*buttonScale, Vector3(scale, scale, scale));
		}

		if (hoveredButton && Input::MouseState(Input::MouseButton::Left, Input::OnPressed) && !isDecided_)
		{
			// これより下でhitActorがそれぞれあらかじめ決められているActorと同じなら設定されているシーンのIDを入れる
			Uint32 targetScene = nextSceneID_;
			if (hoveredButton == toTitleSceneActor_)
			{
				targetScene = titleSceneID_;
			}
			else if (hoveredButton == toRetrySceneActor_)
			{
				targetScene = retrySceneID_;
			}

			// 一度きりにするためフラグを立てる
			isDecided_ = true;

			// タイムスケールを戻す
			world.GetService<GameTimer>()->TimeScale(1.0f);

			// シーンを遷移する
			Scene::Change(targetScene);
		}
	}
}

void GoalSystem::OnEditorTick(float elapsedTime)
{
	// 場所がわかりやすいように球を表示する
	Vector3 center = GetActor().WorldMatrix().Translation();
	GetActor().GetService<DebugDraw>()->Sphere(center, 0.5f, Color(1.0f, 0.0f, 0.0f, 0.5f));
}

void GoalSystem::ApplyResult()
{
	PlayerController* playerController = player_.GetComponent<PlayerController>();

	for (auto& resultChild : resultActor_.Children())
	{
		SeedCore::Name* name = resultChild.GetComponent<SeedCore::Name>();
		if (name->name_ == "UseCostText")
		{
			//使った総コストのテキスト

			SeedCore::Text* text = resultChild.GetComponent<SeedCore::Text>();
			int costI = playerController->GetTotalUseCost();
			text->text_ = SeedCore::String(std::to_string(costI));
			float textSizeX = text->fontSize_ * textSizeXRate;
			int digit = costI <= 0 ? 1 : std::log10(costI) + 1;
			SeedCore::Position* position = resultChild.GetComponent<SeedCore::Position>();
			position->x_ -= textSizeX * (digit - 1);
		}
		else if (name->name_ == "DeathText")
		{
			//死亡回数のテキスト

			SeedCore::Text* text = resultChild.GetComponent<SeedCore::Text>();
			int deathCountI = playerController->GetDeathCount();
			text->text_ = SeedCore::String(std::to_string(deathCountI));
			float textSizeX = text->fontSize_ * textSizeXRate;
			int digit = deathCountI <= 0 ? 1 : std::log10(deathCountI) + 1;
			SeedCore::Position* position = resultChild.GetComponent<SeedCore::Position>();
			position->x_ -= textSizeX * (digit - 1);
		}
		else if(name->name_ == "ClearTimeText")
		{
			//クリアタイムのテキスト

			SeedCore::Text* text = resultChild.GetComponent<SeedCore::Text>();
			float clearTimeF = playerController->GetClearTimer();
			int clearTimeI = static_cast<int>(clearTimeF);
			int minute = clearTimeI / 60;
			int second = clearTimeI % 60;
			std::string str = std::to_string(minute) + "分" + std::to_string(second) + "秒";
			float textSizeX = text->fontSize_ * (textSizeXRate + 0.07f);
			int digit = 2;
			digit += minute <= 0 ? 1 : std::log10(minute) + 1;
			digit += second <= 0 ? 1 : std::log10(second) + 1;
			text->text_ = SeedCore::String(str);
			SeedCore::Position* position = resultChild.GetComponent<SeedCore::Position>();
			position->x_ -= textSizeX * (digit - 1);
		}
    }
}
