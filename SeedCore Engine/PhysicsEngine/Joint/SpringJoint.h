#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* ばねのように、伸び縮みしながらつなぐ部品。
	*
	* @brief ばねでつなぐ部品
	*
	* [EN]
	* A part that joins two bodies like a spring that stretches and squeezes.
	*
	* @brief Joins bodies with a spring
	*/
	class SEEDCORE_API SpringJoint :public SeedScript
	{
	public:
		/// [JP] つなぐかどうか
		/// [EN] Whether to join at all
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = true;

		/// [JP] つなぐ相手のアクター。0 なら世界につなぐ
		/// [EN] The actor to join to; 0 joins to the world
		SC_PAYLOAD_FIELD_EX("接続先アクター", Actor)
		Uint32 connectedActor_ = 0;

		/// [JP] つなぐ点（自分から見た位置）
		/// [EN] The joining point, seen from this actor
		SC_REFLECTION_FIELD_EX("アンカー(ローカル)")
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 一番近づける距離
		/// [EN] The closest it can get
		SC_REFLECTION_CLAMPED_EX("最小距離", 0.0f, 1000.0f)
		Float minDistance_ = 0.0f;

		/// [JP] 一番離れられる距離
		/// [EN] The farthest it can get
		SC_REFLECTION_CLAMPED_EX("最大距離", 0.0f, 1000.0f)
		Float maxDistance_ = 0.0f;

		/// [JP] ばねの硬さ。大きいほど硬い
		/// [EN] How stiff the spring is; bigger is stiffer
		SC_REFLECTION_CLAMPED_EX("剛性(Hz)", 0.0f, 30.0f)
		Float frequency_ = 2.0f;

		/// [JP] 揺れの止まりやすさ
		/// [EN] How quickly the bouncing settles
		SC_REFLECTION_CLAMPED_EX("減衰", 0.0f, 1.0f)
		Float damping_ = 0.5f;

	public:
		/**
		* [JP]
		* 同じアクターの Rigidbody と相手をつなぐ。
		*
		* @brief つなぐ
		* @note  同じアクターに Rigidbody が要る
		*
		* [EN]
		* Joins this actor's Rigidbody to the other one.
		*
		* @brief Makes the joint
		* @note  Needs a Rigidbody on the same actor
		*/
		void OnStart();

		/**
		* [JP]
		* つなぎを外す。
		*
		* @brief つなぎを外す
		*
		* [EN]
		* Removes the joint.
		*
		* @brief Removes the joint
		*/
		void OnDestroy();

	private:
		/// [JP] 物理エンジンの中のつなぎの目印
		/// [EN] The handle of the joint inside the physics engine
		ScConstraintHandle handle_;
	};
	REGISTER_COMPONENT(SpringJoint, "Physics/Joint");
}
