#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>

namespace SeedCore
{
	struct EntityID;

	class World;

	/**
	* [JP]
	* キャラクターが何かにぶつかったことを受け取って、スクリプトに伝える係。
	*
	* @brief キャラクターのぶつかりを伝える係
	*
	* [EN]
	* Receives what a character bumps into and tells the scripts.
	*
	* @brief Reports character hits
	*/
	class JoltCharacterContactListener final
	{
	public:
		/**
		* [JP]
		* 伝える先の世界とアクターを覚えて準備する。
		*
		* @brief 準備する
		* @param world    伝える世界
		* @param entityID キャラクターのアクター
		*
		* [EN]
		* Remembers the world and actor to report to and gets ready.
		*
		* @brief Gets ready
		* @param world    The world to report to
		* @param entityID The character's actor
		*/
		JoltCharacterContactListener(World& world, EntityID entityID);

		/**
		* [JP]
		* 中身を片付ける。
		*
		* @brief 片付ける
		*
		* [EN]
		* Cleans up the inner parts.
		*
		* @brief Cleans up
		*/
		~JoltCharacterContactListener();

		JoltCharacterContactListener(const JoltCharacterContactListener&) = delete;
		JoltCharacterContactListener& operator=(const JoltCharacterContactListener&) = delete;

	public:
		/**
		* [JP]
		* キャラクターに、知らせの受け取り先として登録する。
		*
		* @brief 受け取り先として登録する
		* @param character 登録するキャラクター
		*
		* [EN]
		* Registers itself with the character as the place to send news.
		*
		* @brief Registers as the listener
		* @param character The character to register with
		*/
		void Attach(ScCharacterHandle character);

		/**
		* [JP]
		* 毎回の動きの前に呼んで、伝えた記録を空にする。
		*
		* @brief 伝えた記録を空にする
		*
		* [EN]
		* Called before each move to clear the record of what was reported.
		*
		* @brief Clears the report record
		*/
		void Begin();

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};
}
