#include "CircuitTestSystem.h"
#include "CircuitSystem.h"

#include <SeedCore/ScLog.h>

using namespace SeedCore;

void CircuitTestSystem::OnStart()
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	// 回路が未設定なら自分自身のCircuitSystemを見る
	circuitActor_ = (circuitActorID_ != 0) ? world.FindActor(circuitActorID_) : actor;
	if (!circuitActor_ || !circuitActor_.GetComponent<CircuitSystem>())
	{
		const Name* name = world.GetComponent<Name>(entity);
		SC_LOG_WARNING("[{}] 確認する回路にCircuitSystemがありません", name ? name->name_.c_str() : "名前なし");
	}

	// 動かす前の位置と回転を基準として保存
	Position* position = world.GetComponent<Position>(entity);
	Rotation* rotation = world.GetComponent<Rotation>(entity);
	if (position)
	{
		cachePosition_ = Transform::Vector(*position);
	}
	if (rotation)
	{
		cacheRotation_ = Transform::Quat(*rotation);
	}

	progress_ = 0.0f;
	angle_ = 0.0f;
	wasPowered_ = false;
}

void CircuitTestSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Entity entity = GetActor().GetEntity();

	CircuitSystem* circuit = circuitActor_ ? circuitActor_.GetComponent<CircuitSystem>() : nullptr;
	bool powered = circuit && circuit->IsPowered();

	// 通電状態が変わった瞬間だけログを出す
	if (isLogOutput_ && powered != wasPowered_)
	{
		const Name* name = world.GetComponent<Name>(entity);
		SC_LOG_NOTICE("[{}] 回路 {} → {}", name ? name->name_.c_str() : "名前なし", wasPowered_ ? "ON" : "OFF", powered ? "ON" : "OFF");
	}
	wasPowered_ = powered;

	// 通電中は浮き上がり、切れたら沈む
	if (moveTime_ <= 0.0f)
	{
		progress_ = powered ? 1.0f : 0.0f;
	}
	else if (powered)
	{
		progress_ = Min(progress_ + elapsedTime / moveTime_, 1.0f);
	}
	else
	{
		progress_ = Max(progress_ - elapsedTime / moveTime_, 0.0f);
	}

	// 通電中だけ回り続ける、切れたらその角度で止まる
	if (powered)
	{
		angle_ += ToRadians(rotateSpeed_) * elapsedTime;
	}

	// 行き過ぎて戻るイージングでぴょこっと浮かせる
	float height = Easing::OutBack(0.0f, floatHeight_, progress_);

	Position* position = world.GetComponent<Position>(entity);
	Rotation* rotation = world.GetComponent<Rotation>(entity);
	if (position)
	{
		Transform::Vector(*position, cachePosition_ + Vector3(0.0f, height, 0.0f));
	}
	if (rotation)
	{
		Transform::Quat(*rotation, cacheRotation_ * Quaternion::CreateFromAxisAngle(Vector3::UnitY, angle_));
	}
}

void CircuitTestSystem::OnInspectorGUI()
{
	CircuitSystem* circuit = circuitActor_ ? circuitActor_.GetComponent<CircuitSystem>() : nullptr;
	if (!circuit)
	{
		ImGui::Text("回路: 未接続 (Play中のみ表示)");
		return;
	}

	ImGui::Text("回路: %s", circuit->IsPowered() ? "ON" : "OFF");
	ImGui::Text("進捗: %.2f", progress_);
}
