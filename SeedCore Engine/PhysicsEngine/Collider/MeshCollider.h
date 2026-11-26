#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	class MeshCollision;

	/**
	* [JP]
	* アクターにつけると、メッシュの形で当たり判定ができる部品。
	*
	* @brief メッシュの当たり判定
	*
	* [EN]
	* Put it on an actor to give it collision shaped like a mesh.
	*
	* @brief Collision shaped like a mesh
	*/
	class SEEDCORE_API MeshCollider :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [JP] 当たり判定に使うメッシュのアセット番号
		/// [EN] The asset number of the mesh used for collision
		SC_PAYLOAD_FIELD_EX("コリジョンメッシュ", MeshCollision)
		Uint32 meshID_ = 0;

		/// [JP] 凸包（へこみのない形）にするか
		/// [EN] Whether to wrap it into a convex shape with no dents
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("凸包にする")
		Bool convex_ = false;

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

	private:
		/**
		* [JP]
		* 読み込んだメッシュから形を作って、当たり判定にする。
		*
		* @brief メッシュから当たり判定を作る
		* @param meshCollision 形に使うメッシュ
		* @note  動く Rigidbody では凸包になる
		*
		* [EN]
		* Makes a shape from the loaded mesh and uses it for collision.
		*
		* @brief Makes the collision from a mesh
		* @param meshCollision The mesh to build the shape from
		* @note  A moving Rigidbody always gets a convex shape
		*/
		void Build(const MeshCollision& meshCollision);

		/**
		* [JP]
		* まだ形を作れていないかを返す。
		*
		* @brief  形待ちか調べる
		* @return まだなら true
		*
		* [EN]
		* Returns whether the shape still needs to be made.
		*
		* @brief  Checks if the shape is pending
		* @return True if it is still pending
		*/
		Bool Pending()const;

	private:
		/// [JP] 物理エンジンの中での体の番号
		/// [EN] The body number inside the physics engine
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [JP] 使っている形
		/// [EN] The shape in use
		ScShapeHandle shapeHandle_;

		/// [JP] まだ形を作れていないか
		/// [EN] Whether the shape still needs to be made
		Bool pending_ = true;
	};
	REGISTER_COMPONENT(MeshCollider, "Physics/Collider");
}
