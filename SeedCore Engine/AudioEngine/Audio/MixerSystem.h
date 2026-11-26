#pragma once

namespace SeedCore
{
	/**
	* [JP]
	* 全体の音量や、種類ごとの音量を変える係。
	*
	* @brief 音量をまとめて変える係
	*
	* [EN]
	* Changes the overall volume and the volume of each kind of sound.
	*
	* @brief Changes volumes
	*/
	class SEEDCORE_API MixerSystem
	{
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
		static void MasterVolume(Float volume);

		/**
		* [JP]
		* 種類（カテゴリー）ごとの音量を変える。
		*
		* @brief 種類ごとの音量を変える
		* @param category 種類の名前
		* @param volume   音量
		*
		* [EN]
		* Changes the volume of one kind (category) of sound.
		*
		* @brief Changes a category volume
		* @param category The category name
		* @param volume   The volume
		*/
		static void CategoryVolume(const String& category, Float volume);

		/**
		* [JP]
		* 音量を最初の値に戻す。
		*
		* @brief 音量を元に戻す
		*
		* [EN]
		* Puts the volumes back to their starting values.
		*
		* @brief Resets the volumes
		*/
		static void ResetVolume();

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
		static Bool Save();

	public:
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
		NODISCARD static Float MasterVolume();

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
		NODISCARD static Float CategoryVolume(const String& category);

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
		NODISCARD static const DynamicArray<String>& CategoryNameList();
	};
}
