#include "DoorSystem.h"
#include "CircuitSystem.h"

#include <SeedCore/ScLog.h>
#include <SeedCore/ScDebug.h>

using namespace SeedCore;

void DoorSystem::OnStart()
{
	World& world = GetWorld();
	Actor actor = GetActor();

	// 行列を分解して自分の回転と位置をとる
	Vector3 position, scale;
	Quaternion rotation;
	Matrix worldMatrix = actor.WorldMatrix();
	worldMatrix.Decompose(scale, rotation, position);

	// MoveTargetを使うので、初期位置・回転をキャッシュする
	cachePosition_ = position;
	cacheRotation_ = rotation;

	// 回路のActorを探す
	// IDはリフレクションで入れられる値
	circuitActor_ = world.GetActor(circuitActorID_);
	const Name* name = GetActor().GetComponent<Name>();
	if (!circuitActor_)
	{
		SC_LOG_WARNING("[{}] 起動元回路 ID{} のアクターが見つかりません。無効なActorです。配線は有効ですか？", name ? name->name_.c_str() : "名前なし", circuitActorID_);
	}

	// 初期化しとく
	progress_ = 0.0f;
}

void DoorSystem::OnFixedTick(float elapsedTime)
{
	Actor actor = GetActor();

	// RigidbodyがついていなければKinematicでないので処理を早期終了
	Rigidbody* rigidbody = actor.GetComponent<Rigidbody>();
	if (!rigidbody)
	{
		return;
	}

	CircuitSystem* circuitSystem = circuitActor_.GetComponent<CircuitSystem>();

	// 回路システムコンポーネントがついてるか＋通電してるかでboolの値に
	bool powered = circuitSystem && circuitSystem->IsPowered();

	// progressは0で閉、1で開
	if (powered)
	{
		// 通電しているならprogressを増やす
		progress_ += elapsedTime / openTime_;
	}
	else
	{
		// 通電していないのでprogressを減らす
		progress_ -= elapsedTime / closeTime_;
	}

	// 1を超える可能性があるのでクランプ
	progress_ = Clamp(progress_, 0.0f, 1.0f);

	// イージングで沈む距離を補間
	float distance = Easing::InOutSine(0.0f, sinkDistance_, progress_);

	// 目標位置は補間した距離ぶん初期位置から下げた場所
	Vector3 targetPosition = cachePosition_ - Vector3(0.0f, distance, 0.0f);

	// Kinematicを動かす
	rigidbody->MoveTarget(targetPosition, cacheRotation_, elapsedTime);
}

void DoorSystem::OnEditorTick(float elapsedTime)
{
	// FixedTick側と合わせる
	Vector3 startPosition = GetActor().WorldMatrix().Translation();
	Vector3 endPosition = startPosition - Vector3(0.0f, sinkDistance_, 0.0f);

	// 矢印を描画
	// Play中はEditorTickはアップデートされないので一緒には沈まない
	GetActor().GetService<DebugDraw>()->Arrow(startPosition, endPosition, Color(0.5f, 0.5f, 0.0f, 1.0f), 0.2f);
}