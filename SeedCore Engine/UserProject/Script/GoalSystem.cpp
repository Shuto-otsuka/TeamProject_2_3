#include "GoalSystem.h"

#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScLog.h>

using namespace SeedCore;

void GoalSystem::OnStart()
{
	isGoal_ = false;
	isDecided_ = false;

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
	}
}

void GoalSystem::OnEditorTick(float elapsedTime)
{
	// 場所がわかりやすいように球を表示する
	Position* position = GetWorld().GetComponent<Position>(GetActor().GetEntity());
	GetActor().GetDebugDraw().Sphere(Transform::Vector(*position), 0.5f, Color(1.0f, 0.0f, 0.0f, 0.5f));
}