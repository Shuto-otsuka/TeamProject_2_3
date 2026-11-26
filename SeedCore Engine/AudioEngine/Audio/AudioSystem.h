#pragma once

namespace SeedCore
{
	class World;
	class AudioResource;

	/**
	* [JP]
	* 音の部品をまとめて毎フレーム動かす係。
	*
	* @brief 音の部品をまとめて動かす係
	*
	* [EN]
	* Runs all the sound parts together every frame.
	*
	* @brief Runs all the sound parts
	*/
	class SEEDCORE_API AudioSystem
	{
	public:
		/**
		* [JP]
		* 音の準備が終わっていない部品に、読み込んだ音を渡す。
		*
		* @brief 部品に音を渡す
		* @param audioResource 音を探す場所
		* @param world         部品を探す世界
		*
		* [EN]
		* Hands loaded sounds to the parts that are not set up yet.
		*
		* @brief Hands sounds to the parts
		* @param audioResource Where to look up sounds
		* @param world         The world to look for parts in
		*/
		static void ResolveSound(AudioResource& audioResource, World& world);

		/**
		* [JP]
		* 聞く側と鳴らす側の部品を、毎フレーム更新する。
		*
		* @brief 部品を更新する
		* @param world     部品を探す世界
		* @param deltaTime 前のフレームからの時間
		*
		* [EN]
		* Updates the listener and source parts every frame.
		*
		* @brief Updates the parts
		* @param world     The world to look for parts in
		* @param deltaTime Time since the last frame
		*/
		static void Update(World& world, Float deltaTime);
	};
}
