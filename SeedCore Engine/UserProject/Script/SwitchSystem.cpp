#include "SwitchSystem.h"

#include <SeedCore/ScComponent.h>
#include <SeedCore/ScPhysics.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>

using namespace SeedCore;

void SwitchSystem::OnStart()
{
	// 初期化しとく
	isOn_ = false;
	wasInside_ = false;
	cooltimer_ = 0.0f;
}

void SwitchSystem::OnTick(float elapsedTime)
{
	// Latchは一度OnになったらOffにならないので早期リターンしても問題ない
	if (mode_ == Mode::Latch && isOn_)
	{
		return;
	}

	World& world = GetWorld();
	Actor actor = GetActor();

	// 行列を分解してTransformを取得する
	// 親子付けの影響を反映するため
	Vector3 position, scale;
	Quaternion rotation;
	Matrix worldMatrix = actor.WorldMatrix();
	worldMatrix.Decompose(scale, rotation, position);

	// 中心算出
	Vector3 center = position + Vector3::Transform(detectOffset_, rotation);

	Physics& physics = *actor.GetService<Physics>();
	DynamicArray<EntityID> entityIDs;
	// 種類で処理を切り替える
	// インスペクタで出す値を全体サイズとするので、ここはハーフサイズにする
	switch (shape_)
	{
	case Shape::Sphere:
	{
		// 判定サイズの最大の成分を直径とみなして半径にする
		float radius = Max(detectSize_.x * 0.5f, Max(detectSize_.y * 0.5f, detectSize_.z * 0.5f));
		entityIDs = physics.Overlap(OverlapShapeKind::Sphere, Vector3(radius, 0.0f, 0.0f), center, Quaternion::Identity);
		break;
	}
	case Shape::Box:
	{
		Vector3 halfSize = Vector3(detectSize_.x * 0.5f, detectSize_.y * 0.5f, detectSize_.z * 0.5f);
		entityIDs = physics.Overlap(OverlapShapeKind::Box, halfSize, center, rotation);
		break;
	}
	}

	bool inside = false;

	// ループで取り出す、基本はFloorMoveと同じ
	for (EntityID entityID : entityIDs)
	{
		Actor other = world.GetActor(entityID);
		if (!other)
		{
			continue;
		}

		// 取り出したActorがこのタグを持っているかのループ
		for (const String& tag : targetTags_)
		{
			// タグをもってるか
			// ここで判定するのはインスペクタで設定するタグ、ちゃんとActorにタグはつけとくこと
			if (other.HasTag(tag))
			{
				// 見つかったのでinsideをtrueにしてループを抜ける
				inside = true;
				break;
			}
		}

		// 見つかったので抜ける
		if (inside)
		{
			break;
		}
	}

	// タイマーを進める
	if (cooltimer_ > 0.0f)
	{
		cooltimer_ -= elapsedTime;
	}

	// wasInsideは前フレームに範囲内に対象がいたかの判定
	bool isPressedMoment = inside && !wasInside_;

	// 動作の種類で処理を変える
	switch (mode_)
	{
	case Mode::Momentary:
		// 乗っている間のみOnなので常に代入
		isOn_ = inside;
		break;
	case Mode::Toggle:
		// 押された瞬間に反転し、その瞬間からクールタイム開始
		// クールタイム中に入り直しても無視
		if (isPressedMoment && cooltimer_ <= 0.0f)
		{
			// 反転
			isOn_ = !isOn_;
			cooltimer_ = cooltime_;
		}
		break;
	case Mode::Latch:
		// 押された瞬間に On
		// 以降は先頭の早期 return でここまで来ない
		if (isPressedMoment)
		{
			isOn_ = true;
		}
		break;
	}

	wasInside_ = inside;

	Animator* animator = actor.GetComponent<Animator>();
	if (animator)
	{
		animator->SetBool("IsOn", isOn_);
	}
}

void SwitchSystem::OnEditorTick(float elapsedTime)
{
	// フラグがtrueならデバッグ表示
	if (isDebugAreaDraw_)
	{
		World& world = GetWorld();
		Actor actor = GetActor();

		// Tick側と計算を合わせる
		Vector3 position, scale;
		Quaternion rotation;
		Matrix worldMatrix = actor.WorldMatrix();
		worldMatrix.Decompose(scale, rotation, position);

		Vector3 center = position + Vector3::Transform(detectOffset_, rotation);

		switch (shape_)
		{
		case Shape::Sphere:
		{
			float radius = Max(detectSize_.x * 0.5f, Max(detectSize_.y * 0.5f, detectSize_.z * 0.5f));
			actor.GetService<DebugDraw>()->Sphere(center, radius, Color(0.5f, 0.5f, 0.0f, 1.0f));
			break;
		}
		case Shape::Box:
			actor.GetService<DebugDraw>()->Box(center, rotation, detectSize_, Color(0.5f, 0.5f, 0.0f, 1.0f));
			break;
		}
	}
}

bool SwitchSystem::IsOn()const
{
	return isOn_;
}