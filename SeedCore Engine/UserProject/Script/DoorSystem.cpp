#include "DoorSystem.h"
#include "CircuitSystem.h"
#include "AnnounceController.h"
#include "PlayerController.h"

#include <SeedCore/ScLog.h>
#include <SeedCore/ScDebug.h>
#include<SeedCore/ScPrefab.h>
#include<SeedCore/ScScreen.h>

using namespace SeedCore;

void DoorSystem::OnStart()
{
	World& world = GetWorld();
	Actor actor = GetActor();

	announceActor_ = GetWorld().GetActor("Announce");
	player_ = GetWorld().GetActor("Player");
	thirdPersonCameraActor_ = GetWorld().GetActor("CameraBrain");
	firstPersonCameraActor_ = GetWorld().GetActor("FirstPersonCameraBrain");

	// 行列を分解して自分の回転と位置をとる
	Vector3 position, scale;
	Quaternion rotation;
	Matrix worldMatrix = actor.WorldMatrix();
	worldMatrix.Decompose(position, rotation, scale);

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

	SeedCore::Actor openAudioActor = GetWorld().GetActor("AudioGateOpen");
	if (openAudioActor)openAudio = openAudioActor.GetComponent<SeedCore::AudioSource>();
	SeedCore::Actor closeAudioActor = GetWorld().GetActor("AudioGateClose");
	if (closeAudioActor)closeAudio = closeAudioActor.GetComponent<SeedCore::AudioSource>();

	switchCountTextActor_ = SeedCore::Prefab::Spawn("SwitchCountText.prefab");
}

void DoorSystem::OnTick(float elapsedTime)
{
	//必要なスイッチがどれだけ押されてるかのテキストの表示

	CircuitSystem* circuitSystem = circuitActor_.GetComponent<CircuitSystem>();

	//テキストのアクターがないもしくは回路がないなら終了
	if (!switchCountTextActor_ || !circuitSystem)return;

	SeedCore::Active* switchCountTextActive = switchCountTextActor_.GetComponent<SeedCore::Active>();
	//スイッチが複数必要で、すべて押さないと起動しないやつだけテキストを表示する
	if (circuitSystem->GetNeedCount() < 2 || circuitSystem->logic_ != CircuitSystem::Logic::And)
	{
		switchCountTextActive->active_ = false;
		return;
	}

	SeedCore::Position* myPosition = GetActor().GetComponent<SeedCore::Position>();
	SeedCore::Position* cameraPosition = nullptr;
	SeedCore::Vector3 cameraFront;
	PlayerController* playerController = player_.GetComponent<PlayerController>();

	if (playerController->IsFirstPerson())
	{
		cameraPosition = firstPersonCameraActor_.GetComponent<SeedCore::Position>();
		cameraFront = firstPersonCameraActor_.WorldMatrix().Forward();
	}
	else
	{
		cameraPosition = thirdPersonCameraActor_.GetComponent<SeedCore::Position>();
		cameraFront = thirdPersonCameraActor_.WorldMatrix().Forward();
	}

	SeedCore::Vector3 cameraToBullet = SeedCore::Transform::Vector(*myPosition) - SeedCore::Transform::Vector(*cameraPosition);
	cameraToBullet.Normalize();
	if (cameraFront.Dot(cameraToBullet) < 0.0f)
	{
		//カメラが爆弾と真反対の方向向いてたらテキストを表示しない
		switchCountTextActive->active_ = false;
	}
	else
	{
		switchCountTextActive->active_ = true;

		//テキストを表示したいワールド位置を算出
		Vector3 position, scale;
		Quaternion rotation;
		Matrix worldMatrix = GetActor().WorldMatrix();
		worldMatrix.Decompose(position, rotation, scale);
		float worldPosY = position.y + ((3.0f * scale.y) / 2);
		SeedCore::Vector3 textWorldPos = SeedCore::Vector3(position.x, worldPosY, position.z);

		//スクリーン座標に変換
		SeedCore::Vector2 textScreenPos = SeedCore::ScreenSpace::WorldToScreen(textWorldPos);
		SeedCore::Position* switchCountTextPosition = switchCountTextActor_.GetComponent<SeedCore::Position>();
		switchCountTextPosition->x_ = textScreenPos.x;
		switchCountTextPosition->y_ = textScreenPos.y;

		//必要なスイッチの数と今起動しているスイッチの数を取得
		int needCount = circuitSystem->GetNeedCount();
		//通電している場合起動しているスイッチの数を強制的に必要なスイッチの数とする
		int onCount = circuitSystem->IsPowered() ? circuitSystem->GetNeedCount() : circuitSystem->GetOnCount();

		//テキストをセット
		SeedCore::Text* switchCountText = switchCountTextActor_.GetComponent<SeedCore::Text>();
		if (switchCountText)
		{
			std::string text = std::to_string(onCount) + "/" + std::to_string(needCount);
			switchCountText->text_ = SeedCore::String(text);
			//必要数に達していたら色を変更
			if (onCount >= needCount)
			{
				switchCountText->color_ = SeedCore::Color(1.0f, 1.0f, 0.0f, 1.0f);
			}
			else
			{
				switchCountText->color_ = SeedCore::Color(0.0f, 0.0f, 0.0f, 1.0f);
			}
		}
	}
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

		if (!wasPowerd)
		{
			//開く瞬間
			if(openAudio)openAudio->Play();

			if (openAnnounce && announceActor_)
			{
				//アナウンス
				AnnounceController* announceController = announceActor_.GetComponent<AnnounceController>();
				if (announceController)
					announceController->StartAnnounce(AnnounceController::Type::OPEN_GATE);
			}
		}
	}
	else
	{
		// 通電していないのでprogressを減らす
		progress_ -= elapsedTime / closeTime_;

		if (wasPowerd)
		{
			//閉まる瞬間
			if (closeAudio)closeAudio->Play();
		}
	}

	wasPowerd = powered;

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