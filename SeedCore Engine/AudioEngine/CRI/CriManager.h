#pragma once

namespace SeedCore
{
	struct GameConfig;
	struct EditorConfig;

	/**
	* [JP]
	* 音のライブラリ（CRI）を動かす係。
	* 始める・毎フレーム動かす・終わる、と音量の管理をする。
	*
	* @brief 音のライブラリを動かす係
	*
	* [EN]
	* Runs the sound library (CRI).
	* It starts it, runs it every frame, shuts it down and keeps the volumes.
	*
	* @brief Runs the sound library
	*/
	class SEEDCORE_API CriManager final
	{
	public:
		/**
		* [JP]
		* 中身を作る。
		*
		* @brief 中身を作る
		*
		* [EN]
		* Makes the inner parts.
		*
		* @brief Makes the inner parts
		*/
		CriManager();

		/**
		* [JP]
		* 中身を片付ける。
		*
		* @brief 中身を片付ける
		*
		* [EN]
		* Cleans up the inner parts.
		*
		* @brief Cleans up the inner parts
		*/
		~CriManager();

		CriManager(const CriManager&) = delete;
		CriManager& operator=(const CriManager&) = delete;

	public:
		/**
		* [JP]
		* ゲームの設定を使って、音のライブラリを始める。
		*
		* @brief  ゲーム用に始める
		* @param  gameConfig ゲームの設定
		* @return うまくいったら true
		*
		* [EN]
		* Starts the sound library with the game settings.
		*
		* @brief  Starts for the game
		* @param  gameConfig The game settings
		* @return True if it worked
		*/
		Bool Initialize(const GameConfig& gameConfig);

		/**
		* [JP]
		* エディタの設定を使って、音のライブラリを始める。
		*
		* @brief  エディタ用に始める
		* @param  editorConfig エディタの設定
		* @return うまくいったら true
		*
		* [EN]
		* Starts the sound library with the editor settings.
		*
		* @brief  Starts for the editor
		* @param  editorConfig The editor settings
		* @return True if it worked
		*/
		Bool Initialize(const EditorConfig& editorConfig);

		/**
		* [JP]
		* 音のライブラリを1フレーム分動かす。
		*
		* @brief 1フレーム分動かす
		*
		* [EN]
		* Runs the sound library for one frame.
		*
		* @brief Runs one frame
		*/
		void Execute();

		/**
		* [JP]
		* 音のライブラリを終わらせる。
		*
		* @brief 終わらせる
		*
		* [EN]
		* Shuts the sound library down.
		*
		* @brief Shuts down
		*/
		void Finalize();

	public:
		/**
		* [JP]
		* 全体の音量を変える。
		*
		* @brief 全体の音量を変える
		* @param volume 音量
		*
		* [EN]
		* Changes the overall volume.
		*
		* @brief Changes the overall volume
		* @param volume The volume
		*/
		void MasterVolume(Float volume);

		/**
		* [JP]
		* 全体の音量を返す。
		*
		* @brief  全体の音量を返す
		* @return 全体の音量
		*
		* [EN]
		* Returns the overall volume.
		*
		* @brief  Returns the overall volume
		* @return The overall volume
		*/
		NODISCARD Float MasterVolume()const;

		/**
		* [JP]
		* 元に戻すときに使う、最初の全体の音量を決める。
		*
		* @brief 最初の全体の音量を決める
		* @param volume 最初の音量
		*
		* [EN]
		* Sets the starting overall volume used when resetting.
		*
		* @brief Sets the default overall volume
		* @param volume The starting volume
		*/
		void DefaultMasterVolume(Float volume);

		/**
		* [JP]
		* 最初の全体の音量を返す。
		*
		* @brief  最初の全体の音量を返す
		* @return 最初の音量
		*
		* [EN]
		* Returns the starting overall volume.
		*
		* @brief  Returns the default overall volume
		* @return The starting volume
		*/
		NODISCARD Float DefaultMasterVolume()const;

		/**
		* [JP]
		* 種類ごとの音量を変える。
		*
		* @brief 種類ごとの音量を変える
		* @param category 種類の名前
		* @param volume   音量
		*
		* [EN]
		* Changes the volume of one category.
		*
		* @brief Changes a category volume
		* @param category The category name
		* @param volume   The volume
		*/
		void CategoryVolume(const String& category, Float volume);

		/**
		* [JP]
		* 種類ごとの音量を返す。
		*
		* @brief  種類ごとの音量を返す
		* @param  category 種類の名前
		* @return その種類の音量
		*
		* [EN]
		* Returns the volume of one category.
		*
		* @brief  Returns a category volume
		* @param  category The category name
		* @return That category's volume
		*/
		NODISCARD Float CategoryVolume(const String& category)const;

		/**
		* [JP]
		* 音の種類の名前の一覧を返す。
		*
		* @brief  種類の一覧を返す
		* @return 種類の名前の一覧
		*
		* [EN]
		* Returns the list of category names.
		*
		* @brief  Returns the category list
		* @return The category names
		*/
		NODISCARD const DynamicArray<String>& CategoryNameList()const;

		/**
		* [JP]
		* 音量を全部、最初の値に戻す。
		*
		* @brief 音量を元に戻す
		*
		* [EN]
		* Puts every volume back to its starting value.
		*
		* @brief Resets the volumes
		*/
		void ResetVolume();

	public:
		/**
		* [JP]
		* 保存してある音量をファイルから読む。
		*
		* @brief  音量を読む
		* @return うまくいったら true
		*
		* [EN]
		* Reads the saved volumes from a file.
		*
		* @brief  Loads the volumes
		* @return True if it worked
		*/
		Bool Load();

		/**
		* [JP]
		* 今の音量をファイルに保存する。
		*
		* @brief  音量を保存する
		* @return うまくいったら true
		*
		* [EN]
		* Saves the current volumes to a file.
		*
		* @brief  Saves the volumes
		* @return True if it worked
		*/
		Bool Save();

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};
}
