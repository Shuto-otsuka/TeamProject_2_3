#pragma once
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorRegistry.h>

#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

#include <FoundationEngine/World/ECS/Archetype/ArchetypeStorage.h>
#include <FoundationEngine/World/ECS/SparseSet/SparseSetStorage.h>

#include <FoundationEngine/World/Service.h>

namespace SeedCore
{
	/**
	* [EN]
	* The entry point of one scene: creates and destroys Actors, adds and
	* removes their components, and holds the services each module
	* installs. Storage itself is left to the registries and storages it
	* owns.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのシーンの入口。Actor の生成・破棄、コンポーネントの追加・削除を
	* 行い、各モジュールが組み込むサービスを保持する。格納そのものは
	* 自身が持つレジストリとストレージに任せる。
	*/
	class SEEDCORE_API World
	{
	private:
		friend class WorldAccessor;

	public:
		/**
		* [EN]
		* Destroys every actor, so their behaviours receive OnDestroy while the
		* services they may use still exist.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全 actor を破棄する。ビヘイビアは、使うかもしれないサービスがまだ
		* 存在する間に OnDestroy を受け取る。
		*/
		~World();

	private:
		/**
		* [EN]
		* Creates an actor named name with persistentID, or with a new ID when
		* persistentID is 0; CreateActor(name) and loaded scenes both come here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* name という名前の actor を persistentID で作る。persistentID が 0 なら
		* 新しい ID を使う。CreateActor(name) も、読み込んだシーンもここを通る。
		*/
		Actor CreateActor(String name, Uint32 persistentID);

	public:
		/**
		* [EN]
		* Creates an actor named name as the last scene root, with the
		* components every actor has: Name, Position, Rotation, Scale,
		* WorldMatrix, Active and Bounds.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* name という名前の actor をシーンのルートの末尾に作り、全 actor が持つ
		* コンポーネント（Name、Position、Rotation、Scale、WorldMatrix、Active、
		* Bounds）を付ける。
		*/
		Actor CreateActor(String name);

		/**
		* [EN]
		* Destroys actor: its children move to the scene roots, its behaviours
		* receive OnDestroy, and its components and entity are released.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor を破棄する。子はシーンのルートへ移り、ビヘイビアは OnDestroy を
		* 受け取り、コンポーネントとエンティティは解放される。
		*/
		void DestroyActor(Actor actor);

		/**
		* [EN]
		* Returns the first actor, in storage order, whose Name is name, or an
		* invalid Actor if there is none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 格納の順で最初に見つかった、Name が name の actor を返す。無ければ
		* 無効な Actor。
		*/
		Actor GetActor(const String& name);

		/**
		* [EN]
		* Returns the actor identified by entityID, such as an ID a raycast or
		* an overlap reported, or an invalid Actor if entityID is stale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID が識別する actor を返す。レイキャストやオーバーラップが
		* 返した ID などに使う。entityID が古ければ無効な Actor。
		*/
		Actor GetActor(EntityID entityID);

		/**
		* [EN]
		* Returns the actor holding persistentID, the ID an actor reference
		* field keeps through save and load, or an invalid Actor if none does.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* persistentID を持つ actor を返す。persistentID は、actor を参照する
		* フィールドがセーブとロードを通して持つ ID。誰も持っていなければ
		* 無効な Actor。
		*/
		Actor GetActor(Uint32 persistentID);

		/**
		* [EN]
		* Returns every actor, in hierarchy order (each actor before its
		* children, siblings in order). The array is a copy taken at the call,
		* so destroying actors while walking it does not cut the walk short.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全ての actor を、ヒエラルキーの順（親が子より先、兄弟は並び順）で
		* 返す。配列は呼んだ時点の写しなので、たどっている途中で actor を
		* 破棄しても走査が途中で切れない。
		*/
		DynamicArray<Actor> Actors();

	public:
		/**
		* [EN]
		* Default-constructs component id for entity in the storage its
		* metadata selects and returns it; returns the existing one if entity
		* already has it, or nullptr if entity is stale or id is unregistered.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* metadata が選ぶストレージで、entity にコンポーネント id をデフォルト
		* 構築して返す。既に持っていればそれを返し、entity が古いか id が
		* 登録解除済みなら nullptr。
		*/
		void* AddComponent(Entity entity, ComponentID id);

		/**
		* [EN]
		* Default-constructs a T for entity and returns it; the storage is
		* chosen at compile time from T's declared storage.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity に T をデフォルト構築して返す。ストレージは T が宣言した格納
		* 種別からコンパイル時に選ぶ。
		*/
		template<typename T>
		T* AddComponent(Entity entity)
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return static_cast<T*>(archetypes_.Add(entity.GetID(), id));
			}
			else
			{
				return static_cast<T*>(sparseSets_.Add(entity.GetID(), id));
			}
		}

		/**
		* [EN]
		* Adds a T for entity and assigns value to it, then returns it; if
		* entity already has a T, value overwrites it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity に T を追加して value を代入し、それを返す。既に T を
		* 持っていれば value で上書きする。
		*/
		template<typename T>
			requires(std::is_copy_assignable_v<T>)
		T* AddComponent(Entity entity, const T& value)
		{
			T* component = AddComponent<T>(entity);
			if (component)
			{
				*component = value;
			}
			return component;
		}

		/**
		* [EN]
		* Returns entity's component id, or nullptr if it has none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity のコンポーネント id を返す。持っていなければ nullptr。
		*/
		void* GetComponent(Entity entity, ComponentID id)const;

		/**
		* [EN]
		* Returns entity's T, or nullptr if it has none; the storage is chosen
		* at compile time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity の T を返す。持っていなければ nullptr。ストレージは
		* コンパイル時に選ぶ。
		*/
		template<typename T>
		T* GetComponent(Entity entity)const
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return static_cast<T*>(archetypes_.Data(entity.GetID(), id));
			}
			else
			{
				return static_cast<T*>(sparseSets_.Data(entity.GetID(), id));
			}
		}

		/**
		* [EN]
		* Returns whether entity has component id.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity がコンポーネント id を持っているかどうかを返す。
		*/
		Bool HasComponent(Entity entity, ComponentID id)const;

		/**
		* [EN]
		* Returns whether entity has a T; the storage is chosen at compile
		* time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity が T を持っているかどうかを返す。ストレージはコンパイル時に
		* 選ぶ。
		*/
		template<typename T>
		Bool HasComponent(Entity entity)const
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return archetypes_.Has(entity.GetID(), id);
			}
			else
			{
				return sparseSets_.Has(entity.GetID(), id);
			}
		}

		/**
		* [EN]
		* Destroys entity's component id, if it has one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity がコンポーネント id を持っていれば破棄する。
		*/
		void RemoveComponent(Entity entity, ComponentID id);

		/**
		* [EN]
		* Destroys entity's T, if it has one; the storage is chosen at compile
		* time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entity が T を持っていれば破棄する。ストレージはコンパイル時に選ぶ。
		*/
		template<typename T>
		void RemoveComponent(Entity entity)
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				archetypes_.Remove(entity.GetID(), id);
			}
			else
			{
				sparseSets_.Remove(entity.GetID(), id);
			}
		}

	public:
		/**
		* [EN]
		* Constructs a T service from arguments and adds it to this world,
		* or returns the existing one if T is already added.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* arguments から T サービスを構築してこのワールドに追加する。
		* T が追加済みならその既存のものを返す。
		*/
		template<typename T,typename... Arguments>
		T* AddService(Arguments&&... arguments)
		{
			return service_.Add<T>(std::forward<Arguments>(arguments)...);
		}

		/**
		* [EN]
		* Returns this world's T service, or nullptr if it has not been added.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このワールドの T サービスを返す。追加されていなければ nullptr。
		*/
		template<typename T>
		T* GetService()const
		{
			return service_.Get<T>();
		}

		/**
		* [EN]
		* Destroys and removes this world's T service; does nothing if absent.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このワールドの T サービスを破棄して取り除く。無ければ何もしない。
		*/
		template<typename T>
		void RemoveService()
		{
			service_.Remove<T>();
		}

	private:
		/// [EN] Entity slots and where each entity's components are; declared first so both storages, which refer to it, are destroyed before it.
		/// [JP] エンティティのスロットと、各エンティティのコンポーネントの場所。両ストレージがこれを参照するので、先に宣言してそれらより後に破棄されるようにする。
		EntityRegistry entities_;

		/// [EN] Every actor's record and the hierarchy; refers to entities_, so declared after it.
		/// [JP] 全 actor のレコードと階層。entities_ を参照するので、その後に宣言する。
		ActorRegistry actors_ = ActorRegistry(entities_);

		/// [EN] This world's archetype-stored components.
		/// [JP] このワールドのアーキタイプ格納コンポーネント。
		ArchetypeStorage archetypes_ = ArchetypeStorage(entities_);

		/// [EN] This world's sparse-stored components.
		/// [JP] このワールドのスパース格納コンポーネント。
		SparseSetStorage sparseSets_ = SparseSetStorage(entities_);

		/// [EN] Services each module installs into this world; destroyed in reverse order of addition.
		/// [JP] 各モジュールがこのワールドに組み込むサービス。追加の逆順で破棄される。
		Service service_;
	};
}