#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Collider/CircleCollider.h>
#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Collider/CapsuleCollider.h>
#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Collider/MeshCollider.h>
#include <PhysicsEngine/Softbody/Softbody.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <GraphicsEngine/Model/Collision/MeshCollisionResource.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <PhysicsEngine/Physics/Physics.h>

namespace SeedCore
{
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
	void PhysicsSystem::ApplyTransform(Actor actor, RigidbodyDesc& desc)
	{
		const Position* position = actor.GetComponent<Position>();
		const Rotation* rotation = actor.GetComponent<Rotation>();

		/// [JP] 親の位置と向きをかけて、世界での位置と向きを出す
		/// [EN] Apply the parent's transform to get the world position and facing
		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

		Vector3 origin = Vector3::TransformPosition(position ? Transform::Vector(*position) : Vector3::Zero, parentMatrix);
		Quaternion worldRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

		/// [JP] 2D の当たり判定は、ピクセルをメートルに直し、Y を逆向きにする
		/// [EN] For 2D collision, turn pixels into meters and flip Y
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			SC_CONSTEX Float pixelsPerMeter = 100.0f;

			desc.position_ = Vector3{ origin.x / pixelsPerMeter, -origin.y / pixelsPerMeter, 0.0f };
			desc.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -worldRotation.ToEuler().z);
		}
		else
		{
			desc.position_ = origin;
			desc.rotation_ = worldRotation;
		}

		/// [JP] どのアクターの体か分かるように、番号を入れておく
		/// [EN] Store the actor's number so we know whose body it is
		desc.userData_ = ActorAccessor::GetEntity(actor).GetID();
	}

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
	void PhysicsSystem::ApplyActive(World& world)
	{
		if (!world.GetService<Physics>())
		{
			return;
		}

		Physics& physics = *world.GetService<Physics>();

		/// [JP] アクターが動いていれば体も動かし、止まっていれば体も止める
		/// [EN] Wake the body if its actor is active, otherwise pause it
		for (ScBodyID bodyID : physics.BodyList())
		{
			Actor actor = WorldAccessor::Make(world, physics.BodyEntityID(bodyID));
			if (!actor)
			{
				continue;
			}

			if (actor.Active())
			{
				physics.ResumeBody(bodyID);
			}
			else
			{
				physics.SuspendBody(bodyID);
			}
		}

		/// [JP] つなぎも合わせて更新する
		/// [EN] Update the joints to match
		physics.RefleshJoint();
	}

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
	void PhysicsSystem::ApplyCollider(World& world)
	{
		/// [JP] 当たり判定の種類ごとに、動いているアクターのものを更新する
		/// [EN] For each kind of collider, update those on active actors
		DynamicArray<EntityID> rectColliderEntities;
		Query<Read<RectCollider>>(world).ForEach([&](EntityID entityID, const RectCollider&) { rectColliderEntities.push_back(entityID); });
		for (EntityID id : rectColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			RectCollider* collider = actor.GetComponent<RectCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}

		DynamicArray<EntityID> circleColliderEntities;
		Query<Read<CircleCollider>>(world).ForEach([&](EntityID entityID, const CircleCollider&) { circleColliderEntities.push_back(entityID); });
		for (EntityID id : circleColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CircleCollider* collider = actor.GetComponent<CircleCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}

		DynamicArray<EntityID> boxColliderEntities;
		Query<Read<BoxCollider>>(world).ForEach([&](EntityID entityID, const BoxCollider&) { boxColliderEntities.push_back(entityID); });
		for (EntityID id : boxColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			BoxCollider* collider = actor.GetComponent<BoxCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}

		DynamicArray<EntityID> sphereColliderEntities;
		Query<Read<SphereCollider>>(world).ForEach([&](EntityID entityID, const SphereCollider&) { sphereColliderEntities.push_back(entityID); });
		for (EntityID id : sphereColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			SphereCollider* collider = actor.GetComponent<SphereCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}

		DynamicArray<EntityID> capsuleColliderEntities;
		Query<Read<CapsuleCollider>>(world).ForEach([&](EntityID entityID, const CapsuleCollider&) { capsuleColliderEntities.push_back(entityID); });
		for (EntityID id : capsuleColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CapsuleCollider* collider = actor.GetComponent<CapsuleCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}

		DynamicArray<EntityID> cylinderColliderEntities;
		Query<Read<CylinderCollider>>(world).ForEach([&](EntityID entityID, const CylinderCollider&) { cylinderColliderEntities.push_back(entityID); });
		for (EntityID id : cylinderColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			CylinderCollider* collider = actor.GetComponent<CylinderCollider>();
			if (!collider)
			{
				continue;
			}

			collider->Refresh();
		}
	}

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
	void PhysicsSystem::ApplyRigidbody(World& world, Float elapsedTime)
	{
		/// [JP] まず全部の Rigidbody を更新する
		/// [EN] First update every Rigidbody
		DynamicArray<EntityID> rigidbodyEntities;
		Query<Read<Rigidbody>>(world).ForEach([&](EntityID entityID, const Rigidbody&) { rigidbodyEntities.push_back(entityID); });
		for (EntityID id : rigidbodyEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			Rigidbody* rigidbody = actor.GetComponent<Rigidbody>();
			if (!rigidbody)
			{
				continue;
			}

			rigidbody->Apply(elapsedTime);
		}

		/// [JP] そのあと足場の動きを当てはめる
		/// [EN] Then apply the moving footholds
		for (EntityID id : rigidbodyEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			Rigidbody* rigidbody = actor.GetComponent<Rigidbody>();
			if (!rigidbody)
			{
				continue;
			}

			rigidbody->ApplyFoothold(elapsedTime);
		}
	}

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
	ScBodyID PhysicsSystem::CreateColliderBody(Actor actor, ScShapeHandle shape, Bool isTrigger)
	{
		/// [JP] Rigidbody があれば、そちらが体を持つので作らない
		/// [EN] With a Rigidbody, it already owns the body, so make nothing
		if (actor.GetComponent<Rigidbody>())
		{
			return ScInvalidBodyID;
		}

		/// [JP] 動かない体として設定し、2D や重なり用の層を足す
		/// [EN] Set it up as a still body and add the 2D or overlap layers
		RigidbodyDesc desc;
		desc.shape_ = shape;
		desc.motionType_ = ScMotionType::Static;
		desc.layer_ = Layers::Pack(Layers::STATIC, actor.Layer());

		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			desc.layer_ |= Layers::PLANAR;
		}

		desc.isSensor_ = isTrigger;

		if (isTrigger)
		{
			desc.layer_ |= Layers::SENSOR;
		}

		ApplyTransform(actor, desc);

		return actor.GetService<Physics>()->CreateRigidbody(desc);
	}

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
	void PhysicsSystem::DestroyColliderBody(Actor actor, ScBodyID bodyID, ScShapeHandle shape)
	{
		if (bodyID != ScInvalidBodyID)
		{
			actor.GetService<Physics>()->DestroyBody(bodyID);
		}

		actor.GetService<Physics>()->ReleaseShape(shape);
	}

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
	void PhysicsSystem::DispatchCollisionEnter(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchCollisionEnter(otherActor);
			}
		}
	}

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
	void PhysicsSystem::DispatchCollisionStay(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchCollisionStay(otherActor);
			}
		}
	}

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
	void PhysicsSystem::DispatchCollisionExit(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchCollisionExit(otherActor);
			}
		}
	}

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
	void PhysicsSystem::DispatchTriggerEnter(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchTriggerEnter(otherActor);
			}
		}
	}

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
	void PhysicsSystem::DispatchTriggerStay(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchTriggerStay(otherActor);
			}
		}
	}

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
	void PhysicsSystem::DispatchTriggerExit(World& world, EntityID entityID, EntityID otherEntityID)
	{
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [JP] スクリプトの部品だけを集める
		/// [EN] Gather only the script parts
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

		/// [JP] 集めた部品に知らせる
		/// [EN] Tell each of them
		Entity entity = ActorAccessor::GetEntity(actor);
		for (ComponentID id : behaviours)
		{
			if (void* data = world.GetComponent(entity, id))
			{
				id->hooks_->upcast_(data)->DispatchTriggerExit(otherActor);
			}
		}
	}

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
	void PhysicsSystem::ResolveMeshCollider(MeshCollisionResource& meshCollisionResource, World& world)
	{
		DynamicArray<EntityID> meshColliderEntities;
		Query<Read<MeshCollider>>(world).ForEach([&](EntityID entityID, const MeshCollider&) { meshColliderEntities.push_back(entityID); });
		for (EntityID id : meshColliderEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor)
			{
				continue;
			}

			/// [JP] 形をまだ作れていない部品だけを扱う
			/// [EN] Only handle parts that still need their shape
			MeshCollider* collider = actor.GetComponent<MeshCollider>();
			if (!collider || !collider->Pending())
			{
				continue;
			}

			/// [JP] メッシュがまだ読み込めていなければ、次のフレームでまた試す
			/// [EN] If the mesh is not loaded yet, try again next frame
			Handle<MeshCollision> handle = meshCollisionResource.GetHandle(collider->meshID_);
			if (handle.empty())
			{
				continue;
			}

			MeshCollision* meshCollision = meshCollisionResource.Resolve(handle);
			if (!meshCollision)
			{
				continue;
			}

			collider->Build(*meshCollision);
		}
	}

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
	void PhysicsSystem::ResolveSoftbody(ModelResource& modelResource, World& world)
	{
		DynamicArray<EntityID> softbodyEntities;
		Query<Read<Softbody>>(world).ForEach([&](EntityID entityID, const Softbody&) { softbodyEntities.push_back(entityID); });
		for (EntityID id : softbodyEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor)
			{
				continue;
			}

			/// [JP] まだ作れていない Softbody だけを扱う
			/// [EN] Only handle soft bodies that are not built yet
			Softbody* softbody = actor.GetComponent<Softbody>();
			if (!softbody || !softbody->Pending())
			{
				continue;
			}

			const Mesh* mesh = actor.GetComponent<Mesh>();
			if (!mesh)
			{
				continue;
			}

			/// [JP] モデルがまだ読み込めていなければ、次のフレームでまた試す
			/// [EN] If the model is not loaded yet, try again next frame
			Handle<Crister> handle = modelResource.GetHandle(mesh->meshID_);
			if (handle.empty())
			{
				continue;
			}

			Crister* crister = modelResource.Resolve(handle);
			if (!crister)
			{
				continue;
			}

			softbody->Build(*crister);
		}
	}
}
