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
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <PhysicsEngine/Physics/Physics.h>

namespace SeedCore
{
	/**
	* [EN]
	* Writes the actor's pose and EntityID into desc. Canvas actors (with
	* a Rect/CircleCollider) are converted from pixels to meters on the
	* z = 0 plane; others use their world pose as is. Position and Rotation
	* are relative to the parent, so they are carried into world space by
	* the parent's world matrix first.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の姿勢と EntityID を desc に書き込む。Canvas の Actor
	* (Rect/CircleCollider 付き)はピクセルからメートルへ変換して z = 0 の
	* 平面に置き、それ以外はワールドでの姿勢をそのまま使う。Position と
	* Rotation は親から見た値なので、先に親のワールド行列でワールド空間へ移す。
	*/
	void PhysicsSystem::ApplyTransform(Actor actor, RigidbodyDesc& desc)
	{
		/// [EN] An actor without Position or Rotation is placed at the origin with no rotation.
		/// [JP] Position や Rotation を持たない Actor は、原点に回転無しで置く。
		const Position* position = actor.GetComponent<Position>();
		const Rotation* rotation = actor.GetComponent<Rotation>();

		/// [EN] A root actor's parent space is the world itself.
		/// [JP] ルートの Actor にとって、親の空間はワールドそのもの。
		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);

		Vector3 origin = Vector3::Transform(position ? Transform::Vector(*position) : Vector3::Zero, parentMatrix);
		Quaternion worldRotation = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

		/// [EN] A Rect or Circle collider marks the actor as a 2D canvas actor.
		/// [JP] Rect か Circle のコライダーがあれば、2D Canvas の Actor として扱う。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			/// [EN] Number of canvas pixels in one physics meter.
			/// [JP] 物理の1メートルに当たる Canvas のピクセル数。
			SC_CONSTEX Float pixelsPerMeter = 100.0f;

			/// [EN] Canvas Y points down and physics Y up, so Y and the canvas angle (the rotation about Z) are negated.
			/// [JP] Canvas の Y は下向き、物理の Y は上向きなので、Y と Canvas の角度(Z 軸まわりの回転)の符号を反転する。
			desc.position_ = Vector3{ origin.x / pixelsPerMeter, -origin.y / pixelsPerMeter, 0.0f };
			desc.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -worldRotation.ToEuler().z);
		}
		else
		{
			desc.position_ = origin;
			desc.rotation_ = worldRotation;
		}

		/// [EN] Lets physics queries and contacts find their way back to the actor.
		/// [JP] 物理クエリや接触から Actor へたどり着けるようにする。
		desc.userData_ = ActorAccessor::GetEntity(actor).GetID();
	}

	/**
	* [EN]
	* Suspends the bodies of inactive actors and resumes those of active
	* ones, then enables only the joints whose bodies are both present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 無効な Actor のボディを停止し、有効な Actor のボディを再開したうえで、
	* 両方のボディがそろっているジョイントだけを有効にする。
	*/
	void PhysicsSystem::ApplyActive(World& world)
	{
		/// [EN] A world without a physics facade has no bodies to update.
		/// [JP] 物理ファサードを持たないワールドには、更新するボディが無い。
		if (!world.GetService<Physics>())
		{
			return;
		}

		Physics& physics = *world.GetService<Physics>();

		/// [EN] Walks every body, suspended ones included, so a reactivated actor gets its body back.
		/// [JP] 停止中も含めた全ボディを回り、再び有効になった Actor にボディを戻す。
		for (ScBodyID bodyID : physics.BodyList())
		{
			/// [EN] Bodies with no living actor behind them are left alone.
			/// [JP] 生きている Actor が背後にいないボディには触れない。
			Actor actor = WorldAccessor::Make(world, physics.BodyEntityID(bodyID));
			if (!actor)
			{
				continue;
			}

			/// [EN] Both calls do nothing when the body is already in the wanted state.
			/// [JP] ボディが既に望む状態なら、どちらの呼び出しも何もしない。
			if (actor.Active())
			{
				physics.ResumeBody(bodyID);
			}
			else
			{
				physics.SuspendBody(bodyID);
			}
		}

		/// [EN] Joints follow the bodies, so they are refreshed after all bodies are settled.
		/// [JP] ジョイントはボディに従うので、全ボディが決まってから更新する。
		physics.RefleshJoint();
	}

	/**
	* [EN]
	* Brings the body of every active actor's lone collider in line with
	* the actor: moves it to the actor's current pose and rebuilds its
	* shape when the actor's world scale has changed, so the collider
	* follows an actor that moves or scales after it was created.
	* MeshCollider is left out, since rebuilding a mesh shape is too
	* costly to do whenever the scale changes. Called every frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な全 Actor の単独のコライダーについて、ボディを Actor に合わせる。
	* Actor の今の姿勢へボディを移し、Actor のワールドスケールが変わって
	* いれば形状を作り直す。これにより、生成後に動いたり拡大縮小したり
	* する Actor にコライダーが追従する。メッシュ形状の作り直しはスケールが
	* 変わるたびに行うには重いため、MeshCollider は対象外。毎フレーム呼ばれる。
	*/
	void PhysicsSystem::ApplyCollider(World& world)
	{
		/// [EN] An inactive actor's body is out of the simulation, so it is brought in line once the actor is active again.
		/// [JP] 無効な Actor のボディはシミュレーション外なので、Actor が再び有効になってから合わせる。
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
	* [EN]
	* Hands what code has changed on every active actor's Rigidbody
	* (bodyType_, isTrigger_, and the actor's LinearVelocity and
	* AngularVelocity) to its body, advances each MoveTo by one step of
	* elapsedTime, and then carries each body that follows its foothold
	* along with it. Called before every fixed step.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な全 Actor の Rigidbody について、コードが変えたもの(bodyType_ と
	* isTrigger_ と、Actor の LinearVelocity と AngularVelocity)をボディへ
	* 渡し、MoveTo による移動を elapsedTime の1ステップ分進めたうえで、
	* 足場に追従するボディを足場と一緒に運ぶ。固定ステップの前に毎回呼ばれる。
	*/
	void PhysicsSystem::ApplyRigidbody(World& world, Float elapsedTime)
	{
		DynamicArray<EntityID> rigidbodyEntities;
		Query<Read<Rigidbody>>(world).ForEach([&](EntityID entityID, const Rigidbody&) { rigidbodyEntities.push_back(entityID); });
		for (EntityID id : rigidbodyEntities)
		{
			/// [EN] An inactive actor's body is out of the simulation, so its changes wait until the actor is active again.
			/// [JP] 無効な Actor のボディはシミュレーション外なので、その変更は Actor が再び有効になるまで待つ。
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

		/// [EN] Footholds come after every Apply, so each foothold's velocity for the step (including one set by MoveTo) is final before a body is carried by it.
		/// [JP] 足場の処理は全ての Apply の後に行う。ボディを運ぶ前に、各足場のこのステップの速度(MoveTo によるものも含む)が決まっているようにするため。
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
	* [EN]
	* Creates the static body of a collider on its own. Returns an
	* invalid ID when the actor has a Rigidbody, which then owns the
	* single body and borrows the collider's shape instead.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダー単独の静的ボディを作る。Actor に Rigidbody がある場合は無効な
	* ID を返す。その場合はボディを Rigidbody が1つだけ持ち、コライダーの
	* 形状を借りる。
	*/
	ScBodyID PhysicsSystem::CreateColliderBody(Actor actor, ScShapeHandle shape, Bool isTrigger)
	{
		/// [EN] One actor gets one body; with a Rigidbody present, that body is the Rigidbody's.
		/// [JP] 1つの Actor にボディは1つ。Rigidbody があれば、そのボディは Rigidbody のもの。
		if (actor.GetComponent<Rigidbody>())
		{
			return ScInvalidBodyID;
		}

		/// [EN] A lone collider is not simulated, so it is a static body on the actor's layer; every collider but MeshCollider is moved along with its actor by ApplyCollider.
		/// [JP] 単独のコライダーはシミュレーションされないので、Actor のレイヤー上の静的ボディにする。MeshCollider 以外のコライダーは ApplyCollider が Actor に合わせて動かす。
		RigidbodyDesc desc;
		desc.shape_ = shape;
		desc.motionType_ = ScMotionType::Static;
		desc.layer_ = Layers::Pack(Layers::STATIC, actor.Layer());

		/// [EN] Canvas colliders only collide with other canvas bodies.
		/// [JP] Canvas のコライダーは、他の Canvas のボディとだけ衝突する。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			desc.layer_ |= Layers::PLANAR;
		}

		/// [EN] A trigger reports overlaps but does not push anything.
		/// [JP] トリガーは重なりを知らせるだけで、何も押し返さない。
		desc.isSensor_ = isTrigger;

		/// [EN] A trigger is marked in the layer as well, so the layer filters can let it meet kinematic bodies.
		/// [JP] トリガーはレイヤーにも印を付け、レイヤーフィルターがキネマティックのボディと組ませられるようにする。
		if (isTrigger)
		{
			desc.layer_ |= Layers::SENSOR;
		}

		ApplyTransform(actor, desc);

		return actor.GetService<Physics>()->CreateRigidbody(desc);
	}

	/**
	* [EN]
	* Destroys a collider's own body, if it has one, and releases its
	* reference to the shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダー自身のボディがあれば破棄し、形状への参照を解放する。
	*/
	void PhysicsSystem::DestroyColliderBody(Actor actor, ScBodyID bodyID, ScShapeHandle shape)
	{
		/// [EN] A collider under a Rigidbody has no body of its own.
		/// [JP] Rigidbody の下にあるコライダーは、自分のボディを持たない。
		if (bodyID != ScInvalidBodyID)
		{
			actor.GetService<Physics>()->DestroyBody(bodyID);
		}

		/// [EN] The shape is released either way, since the collider holds its own reference.
		/// [JP] コライダーは自分の参照を持っているので、形状はどちらの場合も解放する。
		actor.GetService<Physics>()->ReleaseShape(shape);
	}

	/**
	* [EN]
	* Tells every component of the entity that a collision with the other
	* entity began.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティとの衝突が
	* 始まったことを伝える。
	*/
	void PhysicsSystem::DispatchCollisionEnter(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Tells every component of the entity that a collision with the other
	* entity continues.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティとの衝突が
	* 続いていることを伝える。
	*/
	void PhysicsSystem::DispatchCollisionStay(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Tells every component of the entity that a collision with the other
	* entity ended.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティとの衝突が
	* 終わったことを伝える。
	*/
	void PhysicsSystem::DispatchCollisionExit(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Tells every component of the entity that the other entity entered a
	* trigger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティがトリガーに
	* 入ったことを伝える。
	*/
	void PhysicsSystem::DispatchTriggerEnter(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Tells every component of the entity that the other entity stays in a
	* trigger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティがトリガーの中に
	* 留まっていることを伝える。
	*/
	void PhysicsSystem::DispatchTriggerStay(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Tells every component of the entity that the other entity left a
	* trigger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティの全コンポーネントへ、相手のエンティティがトリガーから
	* 出たことを伝える。
	*/
	void PhysicsSystem::DispatchTriggerExit(World& world, EntityID entityID, EntityID otherEntityID)
	{
		/// [EN] Either actor may have been destroyed between the contact and its delivery.
		/// [JP] 接触から配送までの間に、どちらかの Actor が破棄されていることがある。
		Actor actor = WorldAccessor::Make(world, entityID);
		Actor otherActor = WorldAccessor::Make(world, otherEntityID);
		if (!actor || !otherActor)
		{
			return;
		}

		/// [EN] Behaviours live in sparse sets, so the entity's sparse locations list them. Only their IDs are copied, and each is looked up again before the call, since a handler may add or remove components.
		/// [JP] ビヘイビアはスパースセットにあるので、エンティティのスパースの場所が一覧になる。受け取った側がコンポーネントを付け外ししうるので、ID だけを写し、呼ぶ前にそれぞれ引き直す。
		DynamicArray<ComponentID> behaviours;
		for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
		{
			if (location.id_->hooks_ && location.id_->hooks_->upcast_)
			{
				behaviours.push_back(location.id_);
			}
		}

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
	* [EN]
	* Builds every MeshCollider still waiting for its collision mesh,
	* once the asset has finished loading. Called every frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 衝突メッシュを待っている MeshCollider を、アセットの読み込みが
	* 終わったものから構築する。毎フレーム呼ばれる。
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

			/// [EN] Only colliders still waiting are processed; built ones are skipped.
			/// [JP] 待っているコライダーだけを処理し、構築済みのものは飛ばす。
			MeshCollider* collider = actor.GetComponent<MeshCollider>();
			if (!collider || !collider->Pending())
			{
				continue;
			}

			/// [EN] An asset ID that is not registered yields no handle.
			/// [JP] 登録されていないアセット ID ではハンドルが得られない。
			Handle<MeshCollision> handle = meshCollisionResource.GetHandle(collider->meshID_);
			if (handle.empty())
			{
				continue;
			}

			/// [EN] Null while the asset is still loading; the collider stays pending and is tried again next frame.
			/// [JP] 読み込み中は null。コライダーは待ちのまま残り、次のフレームで再び試す。
			MeshCollision* meshCollision = meshCollisionResource.Resolve(handle);
			if (!meshCollision)
			{
				continue;
			}

			collider->Build(*meshCollision);
		}
	}

	/**
	* [EN]
	* Builds every Softbody still waiting for the model of its actor's
	* Mesh, once the model has finished loading. Called every frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の Mesh のモデルを待っている Softbody を、モデルの読み込みが
	* 終わったものから構築する。毎フレーム呼ばれる。
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

			/// [EN] Only soft bodies still waiting are processed; built ones are skipped.
			/// [JP] 待っているソフトボディだけを処理し、構築済みのものは飛ばす。
			Softbody* softbody = actor.GetComponent<Softbody>();
			if (!softbody || !softbody->Pending())
			{
				continue;
			}

			/// [EN] The soft body takes its geometry from the Mesh on the same actor.
			/// [JP] ソフトボディは、同じ Actor の Mesh から形を取る。
			const Mesh* mesh = actor.GetComponent<Mesh>();
			if (!mesh)
			{
				continue;
			}

			/// [EN] An asset ID that is not registered yields no handle.
			/// [JP] 登録されていないアセット ID ではハンドルが得られない。
			Handle<Crister> handle = modelResource.GetHandle(mesh->meshID_);
			if (handle.empty())
			{
				continue;
			}

			/// [EN] Null while the model is still loading; the soft body stays pending and is tried again next frame.
			/// [JP] 読み込み中は null。ソフトボディは待ちのまま残り、次のフレームで再び試す。
			Crister* crister = modelResource.Resolve(handle);
			if (!crister)
			{
				continue;
			}

			softbody->Build(*crister);
		}
	}
}
