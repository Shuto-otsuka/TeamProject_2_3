#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Captures a byte-level snapshot of every Actor's component data in
	* a World, and can later restore that data back onto the same
	* actors (e.g. to reset play-mode changes in the editor back to the
	* pre-play state).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World 内の全 Actor のコンポーネントデータを、バイト単位の
	* スナップショットとして保存し、後で同じ actor 群へ復元できる
	* クラス（例: エディタでプレイモードの変更をプレイ前の状態へ
	* リセットするために使う）。
	*/
	class SEEDCORE_API WorldSnapshot
	{
	public:
		/**
		* [EN]
		* Clears any existing snapshot, then captures a copy of every
		* Actor's component data currently in world — both archetype-stored
		* (via its archetype's layout) and sparse-set-stored (Rigidbody,
		* Softbody, Weather, ...; via the entity's own list of sparse
		* locations).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 既存のスナップショットをクリアし、world 内の現在の全 Actor の
		* コンポーネントデータのコピーを取得する — アーキタイプ格納
		* （そのアーキタイプのレイアウト経由）とスパースセット格納
		* （Rigidbody、Softbody、Weather 等。エンティティ自身が持つスパースの
		* 場所の一覧経由）の両方。
		*/
		void Capture(World& world);

		/**
		* [EN]
		* If a snapshot has been captured, copies its component data
		* back onto world's actors (matched by EntityID, so reordering or
		* reparenting during Play does not mix them up) and resets each
		* ComponentBehaviour's lifecycle state, destroys every actor the
		* snapshot does not know (i.e. created after it, such as by
		* SpawnerSystem during Play), then clears the snapshot. Does
		* nothing if no snapshot exists.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スナップショットが取得済みであれば、そのコンポーネントデータを
		* world の actor 群へコピーし戻し（EntityID で対応付けるので、Play 中の
		* 並べ替えや親の付け替えで取り違えない）、各 ComponentBehaviour の
		* ライフサイクル状態をリセットし、スナップショットが知らない actor
		* （それより後に生成されたもの、例えば Play 中の SpawnerSystem による
		* 生成）を全て破棄した上で、スナップショットをクリアする。
		* スナップショットが存在しなければ何もしない。
		*/
		void Restore(World& world);

		/**
		* [EN]
		* Returns whether a snapshot is currently held.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スナップショットを現在保持しているかどうかを返す。
		*/
		Bool HasData()const;

		/**
		* [EN]
		* Destructs and discards all captured component data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みの全コンポーネントデータを破棄・削除する。
		*/
		void Clear();

	private:
		/**
		* [EN]
		* A single captured component's raw byte data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みの単一コンポーネントの、生のバイトデータ。
		*/
		struct ComponentData
		{
			/// [EN] The component's registered ComponentID.
			/// [JP] そのコンポーネントの登録済み ComponentID。
			ComponentID id_;

			/// [EN] The component's raw copied bytes.
			/// [JP] コピーされたコンポーネントの生バイト列。
			DynamicArray<Uint8> data_;
		};

		/**
		* [EN]
		* A single captured Actor's data: its entity, every component's
		* captured bytes, and which of them derive from ComponentBehaviour.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みの単一 Actor のデータ: そのエンティティ、全コンポーネントの
		* 取得済みバイト列、およびそのうちどれが ComponentBehaviour から
		* 派生しているか。
		*/
		struct ActorData
		{
			/// [EN] The actor's entity at capture time, which Restore matches the live actor by.
			/// [JP] 取得時点での actor のエンティティ。Restore はこれで生きている actor と対応付ける。
			EntityID entity_;

			/// [EN] Captured byte data for every component the actor had.
			/// [JP] actor が持っていた全コンポーネントの、取得済みバイトデータ。
			DynamicArray<ComponentData> components_;

			/// [EN] IDs of the components that derive from ComponentBehaviour, for resetting lifecycle state on restore.
			/// [JP] ComponentBehaviour から派生するコンポーネントの ID 一覧。復元時にライフサイクル状態をリセットするために使う。
			DynamicArray<ComponentID> componentBaseIDs_;
		};

		/// [EN] Every captured actor's data.
		/// [JP] 取得済みの全 actor のデータ。
		DynamicArray<ActorData> actors_;

		/// [EN] Whether a snapshot has been captured and not yet restored/cleared.
		/// [JP] スナップショットが取得済みで、まだ復元/クリアされていないかどうか。
		Bool hasData_ = false;
	};
}
