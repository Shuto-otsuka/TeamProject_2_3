#include "FloorMoveSystem.h"
#include "StopController.h"
#include "CircuitSystem.h"

#include <SeedCore/ScDebug.h>
#include <SeedCore/ScLog.h>

using namespace SeedCore;

void FloorMoveSystem::OnStart()
{
	World& world = GetWorld();
	Actor actor = GetActor();

	// これがないと止められない
	stopController_ = actor.GetComponent<StopController>();

	// 行列を分解して要素を取り出す、Startのみなので重くない
	// MoveTargetで今の自分の位置で固定にすると徐々にずれるので、ここで初期値のPositionとRotationをキャッシュする
	Matrix worldMatrix = actor.WorldMatrix();
	Vector3 floorPosition, floorScale;
	Quaternion floorRotation;
	worldMatrix.Decompose(floorScale, floorRotation, floorPosition);
	cachePosition_ = floorPosition;
	cacheRotation_ = floorRotation;

	// 一応初期化する
	startIndex_ = 0;
	pingPongDirection_ = 1;
	progress_ = 0.0f;
	waitTimer_ = 0.0f;

	// 現在のTarget位置は自分の初期値に
	currentTarget_ = cachePosition_;

	// Probeの数で処理を決める
	if (static_cast<int>(localPositions_.size()) < 2)
	{
		// そもそも数が0個か1個なら物理的に移動自体が発生しないのでロックフラグを立てる
		isMoveLock_ = true;
	}
	else
	{
		isMoveLock_ = false;
	}

	if (isCanCircuit_)
	{
		circuitActor_ = world.GetActor(circuitActorID_);
		const Name* name = GetActor().GetComponent<Name>();
		if (!circuitActor_)
		{
			SC_LOG_WARNING("[{}] 起動元回路 ID{} のアクターが見つかりません。無効なActorです。配線は有効ですか？", name ? name->name_.c_str() : "名前なし", circuitActorID_);
		}
	}
}

void FloorMoveSystem::OnFixedTick(float elapsedTime)
{
	// ロックフラグが立ってるならここで処理は終了
	if (isMoveLock_)
	{
		return;
	}

	World& world = GetWorld();
	Actor actor = GetActor();

	Rigidbody* rigidbody = actor.GetComponent<Rigidbody>();
	if (!rigidbody)
	{
		return;
	}

	bool powered = false;
	if(isCanCircuit_)
	{
		CircuitSystem* circuitSystem = circuitActor_.GetComponent<CircuitSystem>();

		// 回路システムコンポーネントがついてるか＋通電してるかでboolの値に
		powered = circuitSystem && circuitSystem->IsPowered();

		// Circuitのみ通電してないときも動くのではじく
		if (!powered && moveType_ != MoveType::Circuit)
		{
			rigidbody->MoveTarget(currentTarget_, cacheRotation_, elapsedTime);
			return;
		}
	}

	// 止められているなら現在の目標位置で固定
	if (stopController_ && stopController_->IsStop())
	{
		rigidbody->MoveTarget(currentTarget_, cacheRotation_, elapsedTime);
		return;
	}

	// 端での待ち時間がまだのこってるなら目標位置で固定
	if (waitTimer_ > 0.0f)
	{
		waitTimer_ -= elapsedTime;

		Vector3 targetPosition = cachePosition_ + localPositions_[startIndex_];
		currentTarget_ = targetPosition;
		rigidbody->MoveTarget(targetPosition, cacheRotation_, elapsedTime);
		return;
	}

	// サーキットの時のみ特殊処理
	if (moveType_ == MoveType::Circuit)
	{
		// 通電中は終端まで、通電してないなら始点へ
		int desiredDirection = powered ? 1 : -1;
		if (desiredDirection != pingPongDirection_)
		{
			if (progress_ > 0.0f)
			{
				startIndex_ += pingPongDirection_;
				progress_ = 1.0f - progress_;
			}
			pingPongDirection_ = desiredDirection;
		}
	}

	// 到着予定の位置のIndexは現在の位置のIndexにPingPongなら後段で決まる-1 or 1 で、それ以外なら +1 で決まる
	int arrivalIndex = startIndex_ + pingPongDirection_;

	// 移動床の初期位置はProbeの配列の現在の始点のPosition
	Vector3 startPosition = localPositions_[startIndex_];

	// もしも予定外の範囲に入ったらロックフラグを立てて終了
	if (arrivalIndex < 0 || arrivalIndex >= static_cast<int>(localPositions_.size()))
	{
		// Circuitは端についたら待つのみ、ロックなし
		if (moveType_ == MoveType::Circuit)
		{
			rigidbody->MoveTarget(currentTarget_, cacheRotation_, elapsedTime);
			return;
		}

		isMoveLock_ = true;
		return;
	}

	// 到着予定の位置は直前で出すIndexに対応するPosition
	Vector3 arrivalPosition = localPositions_[arrivalIndex];

	// 現在位置と到着位置の距離
	float sectionLength = Vector3::Distance(arrivalPosition, startPosition);

	// 距離が0.00001より小さいなら到着したとして進捗を1に
	// それ以外なら進捗を距離に応じて進める
	if (sectionLength < 1e-5f)
	{
		progress_ = 1.0f;
	}
	else
	{
		progress_ += scalarSpeed_ * elapsedTime / sectionLength;
		if (progress_ >= 1.0f)
		{
			progress_ = 1.0f;
		}
	}

	// 目標位置を算出して動かす
	// キャッシュした位置を足してるのは、相対位置なのでワールド位置にするため
	Vector3 targetPosition = cachePosition_ + Vector3::Lerp(startPosition, arrivalPosition, progress_);
	currentTarget_ = targetPosition;
	rigidbody->MoveTarget(targetPosition, cacheRotation_, elapsedTime);

	// 進捗が完了したら
	if (progress_ >= 1.0f)
	{
		// 現在位置のIndexを、到着予定位置のIndexだったものに変える
		startIndex_ = arrivalIndex;
		progress_ = 0.0f;
		switch (moveType_)
		{
		case MoveType::PingPong:
			// 現在位置が終端か、一番初めの位置なら向きを反転
			if ((startIndex_ == static_cast<int>(localPositions_.size()) - 1) || startIndex_ == 0)
			{
				// 1なら-1に、-1なら1にすることで前述処理で適切に移動できる
				pingPongDirection_ *= -1;
				waitTimer_ = waitTime_;
			}
			break;
		case MoveType::Repeat:
			// 現在位置が終端なら、一番初めの位置に戻す
			if (startIndex_ == static_cast<int>(localPositions_.size()) - 1)
			{
				startIndex_ = 0;
				waitTimer_ = waitTime_;
			}
			break;
		case MoveType::Circuit:
			// なにもなし
			break;
		}
	}
}

void FloorMoveSystem::OnEditorTick(float elapsedTime)
{
	Vector3 floorPosition = GetActor().WorldMatrix().Translation();
	for (int index = 0;index < static_cast<int>(localPositions_.size());++index)
	{
		// Probeの位置がわかりやすいように球を描画
		GetActor().GetService<DebugDraw>()->Sphere(localPositions_[index] + floorPosition, 0.1f, Color(1.0f, 0.0f, 0.0f, 1.0f));
		if (isDebugArrowDraw_)
		{
			if (static_cast<int>(localPositions_.size()) > index + 1)
			{
				// 移動する向きの矢印を描画
				Vector3 startPosition = localPositions_[index] + floorPosition;
				Vector3 endPosition = localPositions_[index + 1] + floorPosition;
				GetActor().GetService<DebugDraw>()->Arrow(startPosition, endPosition, Color(0.0f, 1.0f, 1.0f, 1.0f), 0.2f);
			}
		}
	}
}