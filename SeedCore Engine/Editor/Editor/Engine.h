#pragma once
#include <SeedCore/Engine/EngineCore.h>

#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>
#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/Editor.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/Build/HotReload.h>

namespace SeedCore
{
	struct Bootstrap;

	/**
	* [EN]
	* The editor executable's engine: boots the shared EngineCore with the
	* editor's extra steps (ImGui, hot reload, editor UI), runs the editor
	* loop, and saves the editor's settings on shutdown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタ実行ファイルのエンジン。共通の EngineCore を、エディタ用の
	* 追加手順（ImGui、ホットリロード、エディタ UI）付きで起動し、エディタの
	* ループを回し、終了時にエディタの設定を保存する。
	*/
	class Engine :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Constructs an engine that has not booted yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* まだ起動していないエンジンを作る。
		*/
		Engine() = default;

		/**
		* [EN]
		* Destroys the engine; Shutdown has already released everything.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジンを破棄する。中身は Shutdown で既に解放されている。
		*/
		~Engine() = default;

		/**
		* [EN]
		* Loads the settings, opens the window, and runs every boot step
		* as one task graph. Returns false when the window cannot be made.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 設定を読み、ウィンドウを開き、全ての起動手順を1つのタスクグラフ
		* として走らせる。ウィンドウを作れなければ false を返す。
		*/
		Bool Boot(const Bootstrap& bootstrap);

		/**
		* [EN]
		* Runs the editor loop until the window's message queue ends.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ウィンドウのメッセージキューが終わるまでエディタのループを回す。
		*/
		void Run();

		/**
		* [EN]
		* Saves the editor's camera, font scale and last scene, then
		* releases the editor, ImGui and the core in that order.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタのカメラ、フォント倍率、最後の Scene を保存し、続けて
		* エディタ、ImGui、コアの順に解放する。
		*/
		void Shutdown();

	private:
		/// [EN] The boot steps and per-frame work shared with the runtime.
		/// [JP] ランタイムと共通の起動手順と毎フレームの処理。
		EngineCore core_;

		/// [EN] Editor-only settings (camera, font scale, last scene, tool paths).
		/// [JP] エディタ専用の設定（カメラ、フォント倍率、最後の Scene、ツールのパス）。
		EditorConfig editorConfig_;

		/// [EN] Game settings, edited from the editor and shared with the runtime.
		/// [JP] ゲームの設定。エディタから編集し、ランタイムと共有する。
		GameConfig gameConfig_;

		/// [EN] The editor UI; created in the last boot step, once everything it refers to exists.
		/// [JP] エディタの UI。参照するものが全て揃った最後の起動手順で作る。
		ResourcePtr<Editor> editor_;

		/// [EN] ImGui backend the editor UI draws through.
		/// [JP] エディタの UI が描画に使う ImGui のバックエンド。
		ResourcePtr<ImGuiRenderer> imgui_;

		/// [EN] Rebuilds and reloads gameplay code while the editor is running.
		/// [JP] エディタを動かしたまま、ゲームプレイのコードを作り直して読み直す。
		HotReload hotReload_;

		/// [EN] Everything the editor panels share, wired up in the last boot step.
		/// [JP] エディタのパネルが共有するもの全て。最後の起動手順でつなぐ。
		EditorContext editorContext_;

		/// [EN] Camera of the editor view.
		/// [JP] エディタービューのカメラ。
		EditorCamera editorCamera_;

		/// [EN] Mouse and keyboard control of the editor camera.
		/// [JP] エディタのカメラのマウス・キーボード操作。
		EditorCameraController editorCameraController_;

		/// [EN] Camera of the canvas view.
		/// [JP] キャンバスビューのカメラ。
		CanvasCamera canvasCamera_;
	};
}
