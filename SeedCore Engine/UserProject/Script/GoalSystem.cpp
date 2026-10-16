#include "GoalSystem.h"

#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScLog.h>
#include <SeedCore/ScMath.h>
#include <SeedCore/ScInput.h>
#include <SeedCore/ScScene.h>

using namespace SeedCore;

void GoalSystem::OnStart()
{
	isGoal_ = false;
	isDecided_ = false;
	resultTimer_ = 0.0f;

	World& world = GetWorld();

	// 後段処理でスケールを0にするのでここで一応1に戻す
	world.Timer().TimeScale(1.0f);

	// 今回はどのステージでもResultUIを親として子供にそれぞれのUIの構成にする
	// ゴールするまでは表示したくないのでActiveはfalseに
	resultActor_ = world.GetActor("ResultUI");
	resultActor_.Active(false);

	Entity resultEntity = resultActor_.GetEntity();
	Scale* resultScale = world.GetComponent<Scale>(resultEntity);
	if (resultScale)
	{
		// 拡大演出のため初期値を0.05fに、親なので子も追従する
		// Pivotの調整はEditorの方で
		Transform::Vector(*resultScale, Vector3(0.05f, 0.05f, 0.05f));
	}

	// それぞれのUIのActorを取得する
	toNextSceneActor_ = world.GetActor("NextButtonUI");
	toTitleSceneActor_ = world.GetActor("TitleButtonUI");
	toRetrySceneActor_ = world.GetActor("RetryButtonUI");
}

void GoalSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	if (!isGoal_)
	{
		Physics& physics = actor.GetPhysics();
		Position* position = world.GetComponent<Position>(entity);
		Rotation* rotation = world.GetComponent<Rotation>(entity);

		// リフレクションで出す範囲を使って、近くの範囲内のEntityを検出する
		DynamicArray<EntityID> entityIDs = physics.Overlap(OverlapShapeKind::Sphere, Vector3(range_, 0.0f, 0.0f), Transform::Vector(*position), Transform::Quat(*rotation));

		for (EntityID entityID : entityIDs)
		{
			Actor other = world.GetActor(entityID);

			// 範囲内にいるActorの内、Playerタグを持つActorを探し出す
			if (other.HasTag("Player"))
			{
				isGoal_ = true;
				resultActor_.Active(true);
				break;
			}
		}
	}
	else
	{
		// リザルト時はゲームは止まってほしいのでタイムスケール0
		// これより先はタイムスケールに関係ないタイマーを使う
		world.Timer().TimeScale(0.0f);
		resultTimer_ += world.Timer().UnscaledDeltaTime();

		// 上がってちょっと超えて下がる感じの動き
		float scale = Easing::OutBack(0.05f, 1.0f, resultTimer_ / directionTime_, 2.68f);
		Scale* resultScale = world.GetComponent<Scale>(resultActor_.GetEntity());
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
		DynamicArray<EntityID> hitEntityIDs = actor.GetPhysics().Overlap2D(OverlapShapeKind::Rect, Vector2(0.5f, 0.5f), Input::MousePoint(), 0.0f);
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

		float deltaTime = world.Timer().UnscaledDeltaTime();
		// それぞれで倍率を常に掛けるためにループを回す
		for (Actor actor : {toNextSceneActor_, toTitleSceneActor_, toRetrySceneActor_})
		{
			Scale* buttonScale = world.GetComponent<Scale>(actor.GetEntity());
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
			world.Timer().TimeScale(1.0f);

			// シーンを遷移する
			Scene::Change(targetScene);
		}
	}
}

void GoalSystem::OnEditorTick(float elapsedTime)
{
	// 場所がわかりやすいように球を表示する
	Position* position = GetWorld().GetComponent<Position>(GetActor().GetEntity());
	GetActor().GetDebugDraw().Sphere(Transform::Vector(*position), 0.5f, Color(1.0f, 0.0f, 0.0f, 0.5f));
}