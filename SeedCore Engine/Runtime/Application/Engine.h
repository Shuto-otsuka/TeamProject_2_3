#pragma once
#include <SeedCore/Engine/EngineCore.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>

namespace SeedCore
{
	struct Bootstrap;

	/**
	* [EN]
	* The shipped game's engine: boots the shared EngineCore with the
	* game's settings and runs the game loop. Unlike the editor's Engine
	* it has no editor UI, hot reload or editor settings.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 出荷するゲームのエンジン。共通の EngineCore をゲームの設定で起動し、
	* ゲームのループを回す。エディタの Engine と違い、エディタの UI、
	* ホットリロード、エディタの設定を持たない。
	*/
	class Engine :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Loads the game settings, opens the window, and runs every boot
		* step as one task graph. Returns false when any step fails.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームの設定を読み、ウィンドウを開き、全ての起動手順を1つの
		* タスクグラフとして走らせる。どれかの手順が失敗すれば false を返す。
		*/
		Bool Boot(const Bootstrap& bootstrap);

		/**
		* [EN]
		* Runs the game loop until the window closes or the game asks to
		* quit.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ウィンドウが閉じるか、ゲームが終了を求めるまでゲームのループを回す。
		*/
		void Run();

		/**
		* [EN]
		* Releases every module of the core.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コアの全モジュールを解放する。
		*/
		void Shutdown();

	private:
		/// [EN] The boot steps and per-frame work shared with the editor.
		/// [JP] エディタと共通の起動手順と毎フレームの処理。
		EngineCore core_;

		/// [EN] Game settings (resolution, upscaling, vsync, audio).
		/// [JP] ゲームの設定（解像度、アップスケール、垂直同期、オーディオ）。
		GameConfig gameConfig_;
	};
}
