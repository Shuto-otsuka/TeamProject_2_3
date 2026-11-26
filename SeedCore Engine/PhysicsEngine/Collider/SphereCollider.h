#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [JP]
	* アクターにつけると、球の形で当たり判定ができる部品。
	*
	* @brief 球の当たり判定
	*
	* [EN]
	* Put it on an actor to give it collision shaped like a sphere.
	*
	* @brief Collision shaped like a sphere
	*/
	class SEEDCORE_API SphereCollider :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [JP] 半径
		/// [EN] The radius
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("半径", 0.001f, 100.0f)
		Float radius_ = 0.5f;

		/// [JP] ぶつからずに、重なったことだけを知らせるか
		/// [EN] Whether it only reports overlaps instead of blocking
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [JP]
		* 当たり判定の体を作る。
		*
		* @brief 当たり判定を作る
		*
		* [EN]
		* Creates the collision body.
		*
		* @brief Creates the collision
		*/
		void OnAwake();

		/**
		* [JP]
		* 当たり判定の体を消す。
		*
		* @brief 当たり判定を消す
		*
		* [EN]
		* Removes the collision body.
		*
		* @brief Removes the collision
		*/
		void OnDestroy();

		/**
		* [JP]
		* 設定が変わったので、当たり判定を作り直す。
		*
		* @brief 当たり判定を作り直す
		*
		* [EN]
		* The settings changed, so the collision is made again.
		*
		* @brief Remakes the collision
		*/
		void OnRevision();

	public:
		/**
		* [JP]
		* 当たり判定を作り直す。
		*
		* @brief 当たり判定を作り直す
		*
		* [EN]
		* Remakes the collision.
		*
		* @brief Remakes the collision
		*/
		void Dispatch();

	public:
		/**
		* [JP]
		* 今の大きさに合わせた形を作って返す。
		*
		* @brief  形を作る
		* @return 作った形
		*
		* [EN]
		* Makes and returns a shape that fits the current size.
		*
		* @brief  Makes the shape
		* @return The new shape
		*/
		ScShapeHandle GetShapeHandle()const;

	private:
		/**
		* [JP]
		* アクターの位置と向きを当たり判定に伝える。
		* 大きさが変わっていたら形も作り直す。
		*
		* @brief 位置と大きさを合わせる
		*
		* [EN]
		* Sends the actor's position and facing to the collision.
		* If the size changed, the shape is made again too.
		*
		* @brief Matches the position and size
		*/
		void Refresh();

	private:
		/// [JP] 物理エンジンの中での体の番号
		/// [EN] The body number inside the physics engine
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [JP] 使っている形
		/// [EN] The shape in use
		ScShapeHandle shapeHandle_;

		/// [JP] 形を作ったときの大きさ。変わったら作り直す
		/// [EN] The scale the shape was made at; remake it if this changes
		Vector3 shapeScale_ = { 0.0f, 0.0f, 0.0f };
	};
	REGISTER_COMPONENT(SphereCollider, "Physics/Collider");
}
