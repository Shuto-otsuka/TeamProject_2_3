#pragma once

namespace SeedCore
{
	struct EditorContext;

	/**
	* [EN]
	* The editor's UI layer: owns every panel, ticks the editor-side
	* cameras and previews, keeps shared assets in sync, and draws the
	* editor each frame. Its state lives behind a pImpl, so this header
	* pulls in none of the panel headers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタの UI 層。全パネルを所有し、エディタ側のカメラとプレビューを
	* 進め、共有アセットを同期し、毎フレームエディタを描画する。状態は
	* pImpl の奥にあるので、このヘッダはパネルのヘッダを一切取り込まない。
	*/
	class Editor : public NonTransferable
	{
	public:
		/**
		* [EN]
		* Creates every panel and the shared-asset sync against context.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* context に対して全パネルと共有アセットの同期を作る。
		*/
		Editor(EditorContext& context);

		/**
		* [EN]
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~Editor();

		/**
		* [EN]
		* Advances the editor by one frame: pending resize requests,
		* cameras, previews and shared-asset sync.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタを1フレーム進める。保留中のリサイズ要求、カメラ、
		* プレビュー、共有アセットの同期を処理する。
		*/
		void Tick(Float deltaTime);

		/**
		* [EN]
		* Renders the editor, game and canvas views plus open previews,
		* then draws every panel and handles editor shortcuts.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタ・ゲーム・キャンバスの各ビューと開いているプレビューを
		* 描画し、続けて全パネルを描いてエディタのショートカットを処理する。
		*/
		void Draw();

	private:
		class Impl;

		/// [EN] The editor's state, owned through a pointer so the panels stay out of this header.
		/// [JP] エディタの状態。ポインタ越しに所有し、パネルをこのヘッダから外す。
		ResourcePtr<Impl> impl_;
	};
}
