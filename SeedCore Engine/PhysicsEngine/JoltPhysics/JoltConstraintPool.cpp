#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Body/Body.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/Constraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/TwoBodyConstraint.h>
#include <PhysicsEngine/JoltPhysics/JoltConstraintPool.h>

namespace SeedCore
{
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
	JoltConstraintPool::~JoltConstraintPool()
	{
		for (Slot& slot : slots_)
		{
			if (slot.constraint_)
			{
				slot.constraint_->Release();
			}
		}
	}

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
	ScConstraintHandle JoltConstraintPool::Add(JPH::PhysicsSystem& system, JPH::Constraint* constraint)
	{
		if (!constraint)
		{
			return ScConstraintHandle::null();
		}

		/// [JP] 物理エンジンに入れて、空いている入れ物か新しい入れ物にしまう
		/// [EN] Add it to the physics engine and put it in a free or new slot
		system.AddConstraint(constraint);

		Uint64 index = 0;
		if (!freeIndices_.empty())
		{
			index = freeIndices_.back();
			freeIndices_.pop_back();
		}
		else
		{
			index = slots_.size();
			slots_.emplace_back();
		}

		Slot& slot = slots_[index];
		constraint->AddRef();
		slot.constraint_ = constraint;

		/// [JP] 入れ物の番号と使い回し回数を番号にする
		/// [EN] Make the number from the slot index and its reuse count
		ScConstraintHandle handle{};
		handle.index_ = index;
		handle.generation_ = slot.generation_;
		return handle;
	}

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
	void JoltConstraintPool::Release(JPH::PhysicsSystem& system, ScConstraintHandle handle)
	{
		/// [JP] 古い番号や空の入れ物なら何もしない
		/// [EN] Do nothing for an old number or an empty slot
		if (handle.empty() || handle.index_ >= slots_.size())
		{
			return;
		}

		Slot& slot = slots_[handle.index_];
		if (slot.generation_ != handle.generation_ || slot.constraint_ == nullptr)
		{
			return;
		}

		/// [JP] 外して手放し、入れ物を空きに戻す
		/// [EN] Remove it, let it go and mark the slot free
		system.RemoveConstraint(slot.constraint_);
		slot.constraint_->Release();
		slot.constraint_ = nullptr;
		++slot.generation_;
		freeIndices_.push_back(handle.index_);
	}

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
	void JoltConstraintPool::Clear(JPH::PhysicsSystem& system)
	{
		for (Slot& slot : slots_)
		{
			if (slot.constraint_ != nullptr)
			{
				system.RemoveConstraint(slot.constraint_);
				slot.constraint_->Release();
				slot.constraint_ = nullptr;
			}
			++slot.generation_;
		}
		slots_.clear();
		freeIndices_.clear();
	}

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
	void JoltConstraintPool::Refresh()
	{
		for (Slot& slot : slots_)
		{
			if (slot.constraint_ == nullptr || slot.constraint_->GetType() != JPH::EConstraintType::TwoBodyConstraint)
			{
				continue;
			}

			/// [JP] 両方の体が世界にいるかを調べる。世界そのものとつないでいる側はいる扱い
			/// [EN] Check that both bodies are in the world; a side joined to the world itself counts as in
			JPH::TwoBodyConstraint* constraint = static_cast<JPH::TwoBodyConstraint*>(slot.constraint_);
			const JPH::Body* body1 = constraint->GetBody1();
			const JPH::Body* body2 = constraint->GetBody2();

			Bool body1InWorld = body1->GetID().IsInvalid() || body1->IsInBroadPhase();
			Bool body2InWorld = body2->GetID().IsInvalid() || body2->IsInBroadPhase();

			constraint->SetEnabled(body1InWorld && body2InWorld);
		}
	}
}
