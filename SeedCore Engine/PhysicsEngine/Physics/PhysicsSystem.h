#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <PhysicsEngine/Physics/Physics.h>

namespace SeedCore
{
	class Actor;
	class World;
	class MeshCollisionResource;
	class ModelResource;

	/**
	* [JP]
	* 物理の部品をまとめて動かし、ぶつかったことをスクリプトに知らせる係。
	*
	* @brief 物理の部品をまとめて動かす係
	*
	* [EN]
	* Runs the physics parts together and tells scripts when things hit.
	*
	* @brief Runs the physics parts
	*/
	class SEEDCORE_API PhysicsSystem
	{
	public:
		/**
		* [JP]
		* アクターの位置と向きを、物理の体の設定に書き込む。
		*
		* @brief 位置と向きを書き込む
		* @param actor 位置を取るアクター
		* @param desc  書き込む体の設定
		* @note  2D の当たり判定は、ピクセルをメートルに直して Y を逆にする
		*
		* [EN]
		* Writes the actor's position and facing into a body's settings.
		*
		* @brief Writes the position and facing
		* @param actor The actor to read from
		* @param desc  The body settings to write into
		* @note  2D collision turns pixels into meters and flips Y
		*/
		static void ApplyTransform(Actor actor, RigidbodyDesc& desc);

		/**
		* [JP]
		* アクターが動いているかに合わせて、体を止めたり動かしたりする。
		*
		* @brief 体の動く・止まるを合わせる
		* @param world 調べる世界
		*
		* [EN]
		* Pauses or wakes each body to match whether its actor is active.
		*
		* @brief Matches bodies to active actors
		* @param world The world to check
		*/
		static void ApplyActive(World& world);

		/**
		* [JP]
		* 当たり判定の部品に、アクターの位置と大きさを伝える。
		*
		* @brief 当たり判定を更新する
		* @param world 調べる世界
		*
		* [EN]
		* Sends each collider its actor's position and size.
		*
		* @brief Updates the colliders
		* @param world The world to check
		*/
		static void ApplyCollider(World& world);

		/**
		* [JP]
		* Rigidbody の部品を更新する。足場の動きはそのあとでまとめて当てはめる。
		*
		* @brief Rigidbody を更新する
		* @param world       調べる世界
		* @param elapsedTime 前のフレームからの時間
		*
		* [EN]
		* Updates the Rigidbody parts; moving footholds are applied afterwards in one go.
		*
		* @brief Updates the Rigidbodies
		* @param world       The world to check
		* @param elapsedTime Time since the last frame
		*/
		static void ApplyRigidbody(World& world, Float elapsedTime);

		/**
		* [JP]
		* Rigidbody がないアクター用に、当たり判定だけの動かない体を作る。
		*
		* @brief  当たり判定だけの体を作る
		* @param  actor     体を作るアクター
		* @param  shape     使う形
		* @param  isTrigger 重なりを知らせるだけにするか
		* @return 作った体の番号。Rigidbody があれば作らない
		*
		* [EN]
		* Makes a still, collision-only body for an actor without a Rigidbody.
		*
		* @brief  Makes a collision-only body
		* @param  actor     The actor to make it for
		* @param  shape     The shape to use
		* @param  isTrigger Whether it only reports overlaps
		* @return The new body number; none is made if there is a Rigidbody
		*/
		static ScBodyID CreateColliderBody(Actor actor, ScShapeHandle shape, Bool isTrigger = false);

		/**
		* [JP]
		* 当たり判定だけの体と、その形を消す。
		*
		* @brief 当たり判定だけの体を消す
		* @param actor  体を持つアクター
		* @param bodyID 消す体の番号
		* @param shape  返す形
		*
		* [EN]
		* Removes a collision-only body and gives back its shape.
		*
		* @brief Removes a collision-only body
		* @param actor  The actor that owns it
		* @param bodyID The body number to remove
		* @param shape  The shape to give back
		*/
		static void DestroyColliderBody(Actor actor, ScBodyID bodyID, ScShapeHandle shape);

	public:
		/**
		* [JP]
		* ぶつかり始めたことを、アクターのスクリプトに知らせる。
		*
		* @brief ぶつかり始めを知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID ぶつかった相手
		*
		* [EN]
		* Tells an actor's scripts that a hit has started.
		*
		* @brief Reports a hit starting
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it hit
		*/
		static void DispatchCollisionEnter(World& world, EntityID entityID, EntityID otherEntityID);

		/**
		* [JP]
		* ぶつかり続けていることを、アクターのスクリプトに知らせる。
		*
		* @brief ぶつかり中を知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID ぶつかっている相手
		*
		* [EN]
		* Tells an actor's scripts that a hit is still going on.
		*
		* @brief Reports an ongoing hit
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it is hitting
		*/
		static void DispatchCollisionStay(World& world, EntityID entityID, EntityID otherEntityID);

		/**
		* [JP]
		* 離れたことを、アクターのスクリプトに知らせる。
		*
		* @brief 離れたことを知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID 離れた相手
		*
		* [EN]
		* Tells an actor's scripts that a hit has ended.
		*
		* @brief Reports a hit ending
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it stopped hitting
		*/
		static void DispatchCollisionExit(World& world, EntityID entityID, EntityID otherEntityID);

		/**
		* [JP]
		* 重なり始めたことを、アクターのスクリプトに知らせる。
		*
		* @brief 重なり始めを知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID 重なった相手
		*
		* [EN]
		* Tells an actor's scripts that an overlap has started.
		*
		* @brief Reports an overlap starting
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it overlaps
		*/
		static void DispatchTriggerEnter(World& world, EntityID entityID, EntityID otherEntityID);

		/**
		* [JP]
		* 重なり続けていることを、アクターのスクリプトに知らせる。
		*
		* @brief 重なり中を知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID 重なっている相手
		*
		* [EN]
		* Tells an actor's scripts that an overlap is still going on.
		*
		* @brief Reports an ongoing overlap
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it overlaps
		*/
		static void DispatchTriggerStay(World& world, EntityID entityID, EntityID otherEntityID);

		/**
		* [JP]
		* 重なりが終わったことを、アクターのスクリプトに知らせる。
		*
		* @brief 重なりの終わりを知らせる
		* @param world         世界
		* @param entityID      知らせるアクター
		* @param otherEntityID 離れた相手
		*
		* [EN]
		* Tells an actor's scripts that an overlap has ended.
		*
		* @brief Reports an overlap ending
		* @param world         The world
		* @param entityID      The actor to tell
		* @param otherEntityID What it stopped overlapping
		*/
		static void DispatchTriggerExit(World& world, EntityID entityID, EntityID otherEntityID);

	public:
		/**
		* [JP]
		* メッシュの当たり判定がまだ形を作れていなければ、読み込んだメッシュを渡して作らせる。
		*
		* @brief メッシュの当たり判定を仕上げる
		* @param meshCollisionResource メッシュを探す場所
		* @param world                 部品を探す世界
		*
		* [EN]
		* Hands loaded meshes to mesh colliders that have not made their shape yet.
		*
		* @brief Finishes the mesh colliders
		* @param meshCollisionResource Where to look up meshes
		* @param world                 The world to look for parts in
		*/
		static void ResolveMeshCollider(MeshCollisionResource& meshCollisionResource, World& world);

		/**
		* [JP]
		* やわらかい体（Softbody）がまだ作れていなければ、読み込んだモデルを渡して作らせる。
		*
		* @brief Softbody を仕上げる
		* @param modelResource モデルを探す場所
		* @param world         部品を探す世界
		*
		* [EN]
		* Hands loaded models to soft bodies that have not been built yet.
		*
		* @brief Finishes the soft bodies
		* @param modelResource Where to look up models
		* @param world         The world to look for parts in
		*/
		static void ResolveSoftbody(ModelResource& modelResource, World& world);
	};
}
