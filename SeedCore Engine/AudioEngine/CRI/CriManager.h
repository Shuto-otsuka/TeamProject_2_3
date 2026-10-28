#pragma once

namespace SeedCore
{
	struct GameConfig;
	struct EditorConfig;

	/**
	* [EN]
	* Owns and coordinates the CRI ADX runtime.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CRI ADX ランタイムを所有し、連携させる。
	*/
	class SEEDCORE_API CriManager final
	{
	public:
		/**
		* [EN]
		* Constructs an uninitialized manager; Initialize does the work.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未初期化の管理機構を作る。実際の準備は Initialize が行う。
		*/
		CriManager();

		/**
		* [EN]
		* Destroys the manager; Finalize must already have shut CRI down.
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 管理機構を破棄する。CRI の終了は、先に Finalize で済ませておく。
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~CriManager();

		/// [EN] CRI and the Gateway hold pointers into the manager, so it is neither copied nor reassigned.
		/// [JP] CRI と Gateway がマネージャーの中を指すポインタを持つので、コピーも代入もしない。
		CriManager(const CriManager&) = delete;
		CriManager& operator=(const CriManager&) = delete;

	public:
		/**
		* [EN]
		* Initializes CRI, loads the saved bindings, registers the ACF and
		* applies the saved volumes, then allocates the voice pools and the
		* streaming bandwidth manager. Returns false if any step fails.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CRI を初期化し、保存済みの設定を読み、ACF を登録して保存済みの音量を
		* 反映したあと、ボイスプールとストリーミング帯域の管理を確保する。
		* どこかで失敗したら false を返す。
		*/
		Bool Initialize(const GameConfig& gameConfig);

		/**
		* [EN]
		* Same as the GameConfig version, but also takes the .acf file that
		* Atom Craft writes (editorConfig's acfPath_), so a rebuilt ACF
		* replaces the saved copy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GameConfig 版と同じだが、Atom Craft が出力する .acf ファイル
		* (editorConfig の acfPath_)も使い、ビルドし直した ACF で保存済みの
		* ものを置き換える。
		*/
		Bool Initialize(const EditorConfig& editorConfig);

		/**
		* [EN]
		* Runs CRI's per-frame server processing; called once every frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CRI の毎フレームのサーバー処理を行う。毎フレーム1回呼ぶ。
		*/
		void Execute();

		/**
		* [EN]
		* Releases the voice pools, the bandwidth manager and the ACF, and
		* shuts CRI down. Does nothing if CRI was never initialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボイスプール、帯域の管理、ACF を解放し、CRI を終了する。CRI が
		* 初期化されていなければ何もしない。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Sets the master volume and applies it to the master bus at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マスター音量を設定し、すぐにマスターバスへ反映する。
		*/
		void MasterVolume(Float volume);

		/**
		* [EN]
		* Returns the current master volume.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のマスター音量を返す。
		*/
		NODISCARD Float MasterVolume()const;

		/**
		* [EN]
		* Sets the master volume that ResetVolume returns to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResetVolume で戻す先のマスター音量を設定する。
		*/
		void DefaultMasterVolume(Float volume);

		/**
		* [EN]
		* Returns the master volume that ResetVolume returns to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResetVolume で戻す先のマスター音量を返す。
		*/
		NODISCARD Float DefaultMasterVolume()const;

		/**
		* [EN]
		* Sets the volume of a category (such as BGM or SE) and applies it
		* at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カテゴリ(BGM や SE など)の音量を設定し、すぐに反映する。
		*/
		void CategoryVolume(const String& category, Float volume);

		/**
		* [EN]
		* Returns the volume of a category: the value set by the user, else
		* the ACF's value, else 1.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カテゴリの音量を返す。ユーザーが設定した値、無ければ ACF の値、
		* それも無ければ 1。
		*/
		NODISCARD Float CategoryVolume(const String& category)const;

		/**
		* [EN]
		* Returns the names of the categories defined in the registered ACF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 登録した ACF に定義されたカテゴリ名の一覧を返す。
		*/
		NODISCARD const DynamicArray<String>& CategoryNameList()const;

		/**
		* [EN]
		* Returns the master volume to its default and every category to the
		* volume defined in the ACF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マスター音量を既定値に、全カテゴリを ACF で定めた音量に戻す。
		*/
		void ResetVolume();

	public:
		/**
		* [EN]
		* Reads the ACF and the volumes from AudioBindings.scg. Returns false
		* if the file cannot be read.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* AudioBindings.scg から ACF と音量を読み込む。ファイルを読めなければ
		* false を返す。
		*/
		Bool Load();

		/**
		* [EN]
		* Writes the ACF and the volumes to AudioBindings.scg. Returns false
		* if the file cannot be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ACF と音量を AudioBindings.scg へ書き出す。書き込めなければ
		* false を返す。
		*/
		Bool Save();

	private:
		class Impl;

		/// [EN] The CRI runtime state, owned through a pointer so CRI's headers stay out of this one.
		/// [JP] CRI ランタイムの状態。CRI のヘッダをこのヘッダから外すため、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};
}
