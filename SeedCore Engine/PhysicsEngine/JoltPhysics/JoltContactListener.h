#pragma once
#include <FoundationEngine/Utility/ResourcePtr.h>

namespace JPH
{
	class PhysicsSystem;
}

namespace SeedCore
{
	class World;

	/**
	* [JP]
	* 物理エンジンから「ぶつかった」「離れた」の知らせを受け取り、あとでまとめてスクリプトに伝える係。
	*
	* @brief ぶつかりの知らせを集めて伝える係
	*
	* [EN]
	* Receives "hit" and "separated" news from the physics engine and passes it to scripts later, all together.
	*
	* @brief Collects and reports hits
	*/
	class JoltContactListener final
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
		JoltContactListener();

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
		~JoltContactListener();

		JoltContactListener(const JoltContactListener&) = delete;
		JoltContactListener& operator=(const JoltContactListener&) = delete;

	public:
		/**
		* [JP]
		* 物理エンジンに、知らせの受け取り先として登録する。
		*
		* @brief 受け取り先として登録する
		* @param physicsSystem 登録する物理エンジン
		*
		* [EN]
		* Registers itself with the physics engine as the place to send news.
		*
		* @brief Registers as the listener
		* @param physicsSystem The physics engine to register with
		*/
		void Attach(JPH::PhysicsSystem& physicsSystem);

		/**
		* [JP]
		* 集めた知らせを、アクターのスクリプトにまとめて伝える。
		*
		* @brief 知らせを伝える
		* @param physicsSystem 体の状態を調べる物理エンジン
		* @param world         伝える世界
		*
		* [EN]
		* Passes the collected news to the actors' scripts all together.
		*
		* @brief Reports the news
		* @param physicsSystem The physics engine used to check bodies
		* @param world         The world to report to
		*/
		void DispatchEvent(JPH::PhysicsSystem& physicsSystem, World& world);

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};
}
