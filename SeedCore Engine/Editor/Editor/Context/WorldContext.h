#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	struct LoaderSystem;

	class GameTimer;
	class ResourceCache;
	class SystemScheduler;
	class World;

	/**
	* [EN]
	* The world being edited and the services that load into and run it,
	* all owned by Engine and shared with every panel.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 編集中のワールドと、そこへの読み込みや実行を担うサービス。
	* どれも Engine が所有し、全パネルで共有する。
	*/
	struct WorldContext
	{
		/// [EN] The world being edited.
		/// [JP] 編集中のワールド。
		World* world_ = nullptr;

		/// [EN] Asset registry and resource caches.
		/// [JP] アセットの登録簿とリソースのキャッシュ。
		ResourceCache* resource_ = nullptr;

		/// [EN] Asset loader that turns files into resources.
		/// [JP] ファイルをリソースへ読み込むローダー。
		LoaderSystem* loader_ = nullptr;

		/// [EN] Game clock driving Play, Pause and Stop.
		/// [JP] Play・一時停止・停止を駆動するゲームの時計。
		GameTimer* timer_ = nullptr;

		/// [EN] Runs component callbacks and the engine systems each frame.
		/// [JP] コンポーネントのコールバックとエンジンのシステムを毎フレーム実行する。
		SystemScheduler* system_ = nullptr;
	};
}
