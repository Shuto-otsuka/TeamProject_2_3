#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>

namespace SeedCore
{
	struct EntityID;

	class World;

	/**
	* [EN]
	* Bridges Jolt virtual-character contacts to SeedCore collision events
	* for the owning entity and the contacted entity. Only static and
	* kinematic solid bodies are handled here: dynamic bodies and sensors
	* already meet the character's kinematic inner body, so their events
	* come from the world's contact listener instead. Another character is
	* notified by its own listener, so a contact with one is dispatched to
	* the owning entity only. Jolt reports contacts per sub-shape of the
	* contacted body; they are merged per body, so each contacted entity
	* gets one enter, at most one stay per character update, and one exit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt 仮想キャラクターの接触を、所有エンティティと接触先エンティティの
	* SeedCore 衝突イベントへ橋渡しする。ここで扱うのはスタティックと
	* キネマティックの実体のあるボディだけ。動的ボディとセンサーはキャラクターの
	* キネマティックな内側ボディと接触するので、それらのイベントはワールドの
	* 接触リスナーから届く。他のキャラクターへはそのキャラクター自身のリスナーが
	* 通知するので、キャラクターとの接触は所有エンティティにだけ通知する。
	* Jolt は接触を接触先ボディのサブシェイプごとに報告するが、ここではボディ
	* ごとにまとめ、接触先のエンティティごとに Enter を1回、Stay を
	* キャラクターの更新1回につき最大1回、Exit を1回通知する。
	*/
	class JoltCharacterContactListener final
	{
	public:
		/**
		* [EN]
		* Creates a listener that dispatches contacts of the character
		* represented by entityID into world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID が表すキャラクターの接触を world へ通知するリスナーを作る。
		*/
		JoltCharacterContactListener(World& world, EntityID entityID);

		/**
		* [EN]
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~JoltCharacterContactListener();

		/// [EN] Jolt holds a raw pointer to the Impl once attached, so the listener is neither copied nor reassigned.
		/// [JP] 取り付けた後は Jolt が Impl の生ポインタを持つので、リスナーはコピーも代入もしない。
		JoltCharacterContactListener(const JoltCharacterContactListener&) = delete;
		JoltCharacterContactListener& operator=(const JoltCharacterContactListener&) = delete;

	public:
		/**
		* [EN]
		* Registers this listener on the given virtual character, so its
		* contacts start arriving here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定の仮想キャラクターにこのリスナーを登録し、その接触がここへ
		* 届くようにする。
		*/
		void Attach(ScCharacterHandle character);

		/**
		* [EN]
		* Starts a new character update. Each contacted body dispatches at
		* most one enter or stay until the next call. Call it right before
		* the character's update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キャラクターの新しい更新を始める。次に呼ぶまで、接触先ボディごとの
		* Enter か Stay は最大1回になる。キャラクターの更新の直前に呼ぶ。
		*/
		void Begin();

	private:
		class Impl;

		/// [EN] The listener Jolt actually calls, owned through a pointer so its definition can stay in the .cpp.
		/// [JP] Jolt が実際に呼ぶリスナー。定義を .cpp に置けるよう、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};
}
