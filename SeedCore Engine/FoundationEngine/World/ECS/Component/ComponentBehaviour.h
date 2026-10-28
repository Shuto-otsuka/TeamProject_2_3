#pragma once
#include <FoundationEngine/World/ECS/Component/Attachable.h>
#include <FoundationEngine/World/ECS/Component/Detachable.h>
#include <FoundationEngine/World/ECS/Component/Awakeable.h>
#include <FoundationEngine/World/ECS/Component/Startable.h>
#include <FoundationEngine/World/ECS/Component/Tickable.h>
#include <FoundationEngine/World/ECS/Component/FixedTickable.h>
#include <FoundationEngine/World/ECS/Component/LateTickable.h>
#include <FoundationEngine/World/ECS/Component/Editable.h>
#include <FoundationEngine/World/ECS/Component/Destroyable.h>
#include <FoundationEngine/World/ECS/Component/InspectorDrawable.h>
#include <FoundationEngine/World/ECS/Component/Collisionable.h>
#include <FoundationEngine/World/ECS/Component/Triggerable.h>
#include <FoundationEngine/World/ECS/Component/Revisionable.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class Actor;
	class World;

	/**
	* [EN]
	* Base class for class-based components attached to an Actor.
	* Analogous to Unity's MonoBehaviour. Subclasses can optionally
	* implement lifecycle functions (Start, Update, etc.) which are
	* detected and called automatically via concepts.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor にアタッチするクラスベースのコンポーネントの基底クラス。
	* Unity の MonoBehaviour に相当する。サブクラスはライフサイクル関数
	* （Start, Update など）を任意で実装でき、コンセプトを通じて自動的に呼ばれる。
	*/
	class SEEDCORE_API ComponentBehaviour
	{
	private:
		friend class Actor;
		friend class CsharpBehaviour;
		friend class SystemScheduler;
		friend class World;
		friend class WorldSnapshot;

	public:
		/// [EN] Marker tag type used by concepts/traits to detect that a type derives from ComponentBehaviour.
		/// [JP] ある型が ComponentBehaviour から派生していることをコンセプト/トレイトが検出するために使う、マーカータグ型。
		using is_component_behaviour_tag = void;

		/**
		* [EN]
		* Virtual destructor; uses the compiler-generated default.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想デストラクタ。コンパイラ生成のデフォルトを使用する。
		*/
		virtual ~ComponentBehaviour() = default;

		/**
		* [EN]
		* Returns a handle to the Actor this component is attached to, or an
		* invalid Actor if the component has not been attached yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントがアタッチされている Actor へのハンドルを返す。
		* まだアタッチされていなければ無効な Actor。
		*/
		Actor GetActor()const;

		/**
		* [EN]
		* Returns the World that owns this component's Actor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントの Actor を所有する World を返す。
		*/
		World& GetWorld()const;

		/**
		* [EN]
		* Returns the name this component's type is registered under, read
		* from its ComponentRecord.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントの型が登録されている名前を返す。ComponentRecord
		* から読む。
		*/
		const String& GetName()const;

		/**
		* [EN]
		* Dispatches the OnAttach event, run once the moment this component
		* is added to its actor, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントが actor に追加された瞬間に1回だけ走る OnAttach
		* イベントを、このコンポーネントの実装（存在する場合）へディスパッチ
		* する。
		*/
		void DispatchAttach();

		/**
		* [EN]
		* Dispatches the OnDetach event, run the moment this component is
		* removed from its actor, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントが actor から外される瞬間に走る OnDetach イベント
		* を、このコンポーネントの実装（存在する場合）へディスパッチする。
		*/
		void DispatchDetach();

		/**
		* [EN]
		* Dispatches the OnAwake lifecycle event to this component's
		* implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnAwake ライフサイクルイベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchAwake();

		/**
		* [EN]
		* Dispatches the OnStart lifecycle event to this component's
		* implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnStart ライフサイクルイベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchStart();

		/**
		* [EN]
		* Dispatches the OnTick lifecycle event, with the frame's elapsed
		* time, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnTick ライフサイクルイベントを、フレームの経過時間とともに、
		* このコンポーネントの実装（存在する場合）へディスパッチする。
		*/
		void DispatchTick(Float elapsedTime);

		/**
		* [EN]
		* Dispatches the OnFixedTick lifecycle event, with the fixed step's
		* length, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnFixedTick ライフサイクルイベントを、固定ステップの長さとともに、
		* このコンポーネントの実装（存在する場合）へディスパッチする。
		*/
		void DispatchFixedTick(Float elapsedTime);

		/**
		* [EN]
		* Dispatches the OnLateTick lifecycle event, with the frame's
		* elapsed time, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnLateTick ライフサイクルイベントを、フレームの経過時間とともに、
		* このコンポーネントの実装（存在する場合）へディスパッチする。
		*/
		void DispatchLateTick(Float elapsedTime);

		/**
		* [EN]
		* Dispatches the OnEditorTick lifecycle event, with the editor's
		* elapsed time, to this component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnEditorTick ライフサイクルイベントを、エディタの経過時間とともに、
		* このコンポーネントの実装（存在する場合）へディスパッチする。
		*/
		void DispatchEditorTick(Float elapsedTime);

		/**
		* [EN]
		* Dispatches the OnDestroy lifecycle event to this component's
		* implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OnDestroy ライフサイクルイベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchDestroy();

		/**
		* [EN]
		* Dispatches the inspector-GUI draw call to this component's
		* implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* インスペクタ GUI の描画呼び出しを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchInspectorGUI();

		/**
		* [EN]
		* Dispatches a collision-enter event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other との衝突開始イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchCollisionEnter(const Actor& other);

		/**
		* [EN]
		* Dispatches a collision-stay event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other との衝突継続イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchCollisionStay(const Actor& other);

		/**
		* [EN]
		* Dispatches a collision-exit event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other との衝突終了イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchCollisionExit(const Actor& other);

		/**
		* [EN]
		* Dispatches a trigger-enter event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other とのトリガー開始イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchTriggerEnter(const Actor& other);

		/**
		* [EN]
		* Dispatches a trigger-stay event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other とのトリガー継続イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchTriggerStay(const Actor& other);

		/**
		* [EN]
		* Dispatches a trigger-exit event involving other to this
		* component's implementation, if any.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other とのトリガー終了イベントを、このコンポーネントの実装
		* （存在する場合）へディスパッチする。
		*/
		void DispatchTriggerExit(const Actor& other);

		/**
		* [EN]
		* Dispatches a revision event to this component's implementation, if
		* any, so it can rebuild what it made from its fields.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Revision イベントを、このコンポーネントの実装（存在する場合）へ
		* ディスパッチし、フィールドから作ったものを作り直させる。
		*/
		void DispatchRevision();

	private:
		/**
		* [EN]
		* Resets the awoken_/started_ flags, allowing Awake/Start to fire
		* again as if the component were freshly attached.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* awoken_/started_ フラグをリセットし、コンポーネントが新たに
		* アタッチされたかのように Awake/Start を再度発火できるようにする。
		*/
		void ResetLifecycle();

	private:
		/// [EN] The World that owns the actor this component is attached to; set when the component is attached.
		/// [JP] このコンポーネントがアタッチされている actor を所有する World。アタッチ時に設定される。
		World* world_ = nullptr;

		/// [EN] The entity of the actor this component is attached to; set when the component is attached.
		/// [JP] このコンポーネントがアタッチされている actor のエンティティ。アタッチ時に設定される。
		Entity entity_;

		/// [EN] The registration of this component's type, set when the component is attached; its name and hook table are read through it, so the instance keeps no copies of either.
		/// [JP] このコンポーネントの型の登録。アタッチ時に設定される。名前とフックの表はここを通して読むので、インスタンスはどちらの写しも持たない。
		ComponentID id_ = nullptr;

		/// [EN] Whether this component's Awake lifecycle function has already fired.
		/// [JP] このコンポーネントの Awake ライフサイクル関数が既に発火済みかどうか。
		Bool awoken_ = false;

		/// [EN] Whether this component's Start lifecycle function has already fired.
		/// [JP] このコンポーネントの Start ライフサイクル関数が既に発火済みかどうか。
		Bool started_ = false;
	};
}
