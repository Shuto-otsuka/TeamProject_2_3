#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/Utility/Handle.h>

namespace JPH
{
	class PhysicsSystem;
	class Constraint;
}

namespace SeedCore
{
	/**
	* [JP]
	* つなぎ（ジョイント）をまとめて持ち、番号で出し入れする係。
	* 消した番号は使い回し、古い番号を使っても間違えないようにする。
	*
	* @brief つなぎをまとめて持つ係
	*
	* [EN]
	* Holds joints together and hands them out by number.
	* Freed numbers are reused, and an old number is safely ignored.
	*
	* @brief Holds the joints
	*/
	class JoltConstraintPool
	{
	public:
		JoltConstraintPool() = default;

		/**
		* [JP]
		* 持っているつなぎを全部手放す。
		*
		* @brief つなぎを手放す
		*
		* [EN]
		* Lets go of every joint it holds.
		*
		* @brief Lets go of the joints
		*/
		~JoltConstraintPool();

		JoltConstraintPool(const JoltConstraintPool&) = delete;

		JoltConstraintPool& operator=(const JoltConstraintPool&) = delete;

		/**
		* [JP]
		* つなぎを物理エンジンに入れて、番号をつけて持つ。
		*
		* @brief  つなぎを足す
		* @param  system     入れる物理エンジン
		* @param  constraint 足すつなぎ
		* @return つけた番号。つなぎがなければ空
		*
		* [EN]
		* Puts a joint into the physics engine and keeps it under a number.
		*
		* @brief  Adds a joint
		* @param  system     The physics engine to add it to
		* @param  constraint The joint to add
		* @return Its number, or empty if there was no joint
		*/
		ScConstraintHandle Add(JPH::PhysicsSystem& system, JPH::Constraint* constraint);

		/**
		* [JP]
		* 番号のつなぎを物理エンジンから外して手放す。
		*
		* @brief つなぎを外す
		* @param system 外す物理エンジン
		* @param handle 外すつなぎの番号
		*
		* [EN]
		* Takes the numbered joint out of the physics engine and lets it go.
		*
		* @brief Removes a joint
		* @param system The physics engine to remove it from
		* @param handle The number of the joint to remove
		*/
		void Release(JPH::PhysicsSystem& system, ScConstraintHandle handle);

		/**
		* [JP]
		* つなぎを全部外して、空にする。
		*
		* @brief 全部外す
		* @param system 外す物理エンジン
		*
		* [EN]
		* Removes every joint and empties the pool.
		*
		* @brief Removes everything
		* @param system The physics engine to remove them from
		*/
		void Clear(JPH::PhysicsSystem& system);

		/**
		* [JP]
		* つないでいる体が両方とも世界にいるときだけ、つなぎを効かせる。
		*
		* @brief つなぎの有効・無効を合わせる
		*
		* [EN]
		* Turns each joint on only while both of its bodies are in the world.
		*
		* @brief Turns joints on or off
		*/
		void Refresh();

	private:
		/**
		* [JP]
		* つなぎ1つ分の入れ物。
		*
		* @brief つなぎの入れ物
		*
		* [EN]
		* A box that holds one joint.
		*
		* @brief A joint slot
		*/
		struct Slot
		{
			/// [JP] 入っているつなぎ。空なら nullptr
			/// [EN] The joint inside, or nullptr when empty
			JPH::Constraint* constraint_ = nullptr;

			/// [JP] 使い回した回数。古い番号を見分けるのに使う
			/// [EN] How many times it was reused, to spot old numbers
			Uint64 generation_ = 0;
		};

		/// [JP] つなぎの入れ物の一覧
		/// [EN] Every joint slot
		DynamicArray<Slot> slots_;

		/// [JP] 空いている入れ物の番号
		/// [EN] The numbers of empty slots
		DynamicArray<Uint64> freeIndices_;
	};
}
