#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Core/Array.h>
#include <External/JoltPhysics/Jolt/Math/Float3.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Body/Body.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyCreationSettings.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyInterface.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyLock.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyLockMulti.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyManager.h>
#include <External/JoltPhysics/Jolt/Physics/Body/MassProperties.h>
#include <External/JoltPhysics/Jolt/Physics/Body/MotionProperties.h>
#include <External/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/CastResult.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/CollideShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ObjectLayer.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/RayCast.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ShapeCast.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/Shape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/BoxShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/SphereShape.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/Constraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/TwoBodyConstraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/DistanceConstraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/FixedConstraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/HingeConstraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/SliderConstraint.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/SpringSettings.h>
#include <External/JoltPhysics/Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <External/JoltPhysics/Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>
#include <External/JoltPhysics/Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/JoltPhysics/JoltManager.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <PhysicsEngine/JoltPhysics/JoltShapePool.h>
#include <PhysicsEngine/JoltPhysics/JoltConstraintPool.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/Interop/QueryInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a physics facade connected to the engine Jolt manager,
	* recording every query into queryInstance.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンジンのJolt管理機構へ接続された物理ファサードを構築する。
	* クエリはすべて queryInstance に記録する。
	*/
	Physics::Physics(World& world, QueryInstance& queryInstance) :joltManager_(Gateway::GetJoltManager()), world_(world), queryInstance_(queryInstance)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates or reuses a box collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボックス衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateBoxShape(const Vector3& size, const Vector3& center)
	{
		return joltManager_.ShapePool().CreateBoxShape(size, center);
	}

	/**
	* [EN]
	* Creates or reuses a sphere collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 球衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateSphereShape(Float radius)
	{
		return joltManager_.ShapePool().CreateSphereShape(radius);
	}

	/**
	* [EN]
	* Creates or reuses a capsule collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カプセル衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateCapsuleShape(Float height, Float radius)
	{
		return joltManager_.ShapePool().CreateCapsuleShape(height, radius);
	}

	/**
	* [EN]
	* Creates or reuses a cylinder collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 円柱衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateCylinderShape(Float height, Float radius)
	{
		return joltManager_.ShapePool().CreateCylinderShape(height, radius);
	}

	/**
	* [EN]
	* Creates or reuses a rectangular 2D collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 矩形の2D衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateRectShape(const Vector2& size, const Vector2& center)
	{
		/// [EN] Canvas pixels (Y down) become physics meters (Y up), so the center's Y is negated.
		/// [JP] Canvas のピクセル(Y 下向き)を物理のメートル(Y 上向き)へ直すため、中心の Y を反転する。
		return joltManager_.ShapePool().CreateRectShape(Vector2(size.x / pixelsPerMeter_, size.y / pixelsPerMeter_), Vector2(center.x / pixelsPerMeter_, -center.y / pixelsPerMeter_));
	}

	/**
	* [EN]
	* Creates or reuses a circular 2D collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 円形の2D衝突形状を生成または再利用する。
	*/
	ScShapeHandle Physics::CreateCircleShape(Float radius, const Vector2& center)
	{
		/// [EN] Same pixel-to-meter conversion as CreateRectShape.
		/// [JP] CreateRectShape と同じ、ピクセルからメートルへの変換。
		return joltManager_.ShapePool().CreateCircleShape(radius / pixelsPerMeter_, Vector2(center.x / pixelsPerMeter_, -center.y / pixelsPerMeter_));
	}

	/**
	* [EN]
	* Creates or reuses a triangle mesh collision shape, with its
	* vertices scaled per axis. Each asset and scale pair is a separate
	* shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 頂点を軸ごとに拡縮した三角形メッシュ衝突形状を生成または再利用する。
	* アセットとスケールの組ごとに別の形状になる。
	*/
	ScShapeHandle Physics::CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale)
	{
		return joltManager_.ShapePool().CreateMeshShape(assetID, positions, indices, scale);
	}

	/**
	* [EN]
	* Creates or reuses a convex hull collision shape, with its points
	* scaled per axis. Each asset and scale pair is a separate shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 点を軸ごとに拡縮した凸包衝突形状を生成または再利用する。アセットと
	* スケールの組ごとに別の形状になる。
	*/
	ScShapeHandle Physics::CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale)
	{
		return joltManager_.ShapePool().CreateConvexShape(assetID, positions, scale);
	}

	/**
	* [EN]
	* Releases one reference to a pooled collision shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プールされた衝突形状への参照を一つ解放する。
	*/
	void Physics::ReleaseShape(ScShapeHandle handle)
	{
		joltManager_.ShapePool().Release(handle);
	}

	/**
	* [EN]
	* Creates a virtual character from the supplied description, together
	* with a kinematic inner body that represents it in the physics world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した情報から仮想キャラクターを生成し、物理ワールドでその代わりと
	* なるキネマティックな内側ボディも合わせて生成する。
	*/
	ScCharacterHandle Physics::CreateCharacter(const CharacterDesc& desc)
	{
		/// [EN] Jolt's capsule takes the half height of its cylinder part; the caps add radius at each end.
		/// [JP] Jolt のカプセルは円柱部分の半分の高さを取り、両端に半径分の半球が付く。
		JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(desc.height_ * 0.5f, desc.radius_);

		/// [EN] Lift the capsule so its bottom sits on the character's origin, i.e. the origin is the feet.
		/// [JP] カプセルの底がキャラクターの原点に来るよう持ち上げ、原点を足元にする。
		JPH::RefConst<JPH::Shape> shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0.0f, desc.height_ * 0.5f + desc.radius_, 0.0f), JPH::Quat::sIdentity(), capsule);

		/// [EN] Slope limit, mass and the maximum push force applied to other bodies.
		/// [JP] 登れる傾斜の上限、質量、他のボディを押す力の上限。
		JPH::CharacterVirtualSettings settings;
		settings.mShape = shape;
		settings.mMaxSlopeAngle = desc.maxSlopeAngle_;
		settings.mMass = desc.mass_;
		settings.mMaxStrength = desc.maxStrength_;

		/// [EN] The character itself is not a body in the physics world, so a kinematic inner body with the same shape follows it and stands in for it in queries and in contacts with other bodies. It keeps the actor's layer but is classified as KINEMATIC, the motion type it actually has.
		/// [JP] キャラクター自体は物理ワールドのボディではないため、同じ形状のキネマティックな内側ボディを追従させ、クエリや他のボディとの接触でキャラクターの代わりにする。アクターのレイヤーはそのままに、実際の運動タイプである KINEMATIC として分類する。
		settings.mInnerBodyShape = settings.mShape;
		settings.mInnerBodyLayer = Layers::Pack(Layers::KINEMATIC, Layers::UnpackUserLayer(desc.layer_));

		/// [EN] Removes contacts with the internal edges between adjacent triangles, so the character slides smoothly over seams in a mesh.
		/// [JP] 隣り合う三角形の間にある内部エッジとの接触を取り除き、メッシュの継ぎ目をなめらかに移動できるようにする。
		settings.mEnhancedInternalEdgeRemoval = true;

		/// [EN] The EntityID is stored bit for bit as the character's user data; Jolt copies it to the inner body as well, so a query or contact that hits the inner body resolves to this character's entity.
		/// [JP] EntityID はビットのままキャラクターのユーザーデータに格納する。Jolt が内側ボディにも同じ値を写すので、内側ボディに当たったクエリや接触はこのキャラクターのエンティティへ解決される。
		JPH::CharacterVirtual* character = new JPH::CharacterVirtual(&settings, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), std::bit_cast<JPH::uint64>(desc.userData_), &joltManager_.PhysicsSystem());
		character->AddRef();
		return character;
	}

	/**
	* [EN]
	* Advances character movement and stair handling for one frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1フレーム分のキャラクター移動と段差処理を更新する。
	*/
	void Physics::UpdateCharacter(ScCharacterHandle handle, Float elapsedTime, Float maxSlopeAngle, Float stepHeight)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return;
		}

		/// [EN] Applied every frame so changes to the slope limit take effect immediately.
		/// [JP] 傾斜の上限の変更がすぐ反映されるよう、毎フレーム設定する。
		character->SetMaxSlopeAngle(maxSlopeAngle);

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Steps up to stepHeight are climbed instead of blocking the character.
		/// [JP] stepHeight までの段差は、行く手を塞がずに登らせる。
		JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
		updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, stepHeight, 0.0f);

		/// [EN] Moves under world gravity, colliding as a DYNAMIC body; no body or shape filters.
		/// [JP] ワールドの重力で移動し、DYNAMIC ボディとして衝突する。ボディや形状のフィルタは使わない。
		character->ExtendedUpdate(elapsedTime, physicsSystem.GetGravity(), updateSettings, physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator());
	}

	/**
	* [EN]
	* Refreshes the contacts of a virtual character.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 仮想キャラクターの接触情報を更新する。
	*/
	void Physics::RefreshCharacter(ScCharacterHandle handle)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return;
		}

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Re-collects contacts at the current position with the same filters as UpdateCharacter.
		/// [JP] UpdateCharacter と同じフィルタで、現在位置の接触を集め直す。
		character->RefreshContacts(physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator());
	}

	/**
	* [EN]
	* Releases a virtual character reference.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 仮想キャラクターへの参照を解放する。
	*/
	void Physics::DestroyCharacter(ScCharacterHandle& handle)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (character)
		{
			character->Release();
			handle = nullptr;
		}
	}

	/**
	* [EN]
	* Replaces the character shape and its inner body's shape using the
	* specified height and radius. Returns false and leaves both unchanged
	* if the new shape does not fit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した高さと半径で、キャラクターとその内側ボディの形状を置き換える。
	* 新しい形状が収まらない場合は、どちらも変えずに false を返す。
	*/
	Bool Physics::CharacterHeight(ScCharacterHandle handle, Float height, Float radius)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return false;
		}

		/// [EN] Builds the same feet-origin capsule as CreateCharacter.
		/// [JP] CreateCharacter と同じ、足元を原点とするカプセルを作る。
		JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(height * 0.5f, radius);
		JPH::RefConst<JPH::Shape> shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0.0f, height * 0.5f + radius, 0.0f), JPH::Quat::sIdentity(), capsule);

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Refused (false) if the new shape would penetrate the surroundings deeper than 0.05 m.
		/// [JP] 新しい形状が周囲へ 0.05 m より深くめり込む場合は、置き換えずに false を返す。
		if (!character->SetShape(shape, 0.05f, physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator()))
		{
			return false;
		}

		/// [EN] The inner body takes the new shape too, so queries and other bodies see the same height as the character.
		/// [JP] 内側ボディにも新しい形状を与え、クエリや他のボディからもキャラクターと同じ高さに見えるようにする。
		character->SetInnerBodyShape(shape);
		return true;
	}

	/**
	* [EN]
	* Returns the gravity vector used by the physics world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理ワールドで使用する重力ベクトルを返す。
	*/
	Vector3 Physics::Gravity()const
	{
		/// [EN] Converts Jolt's vector to the engine's Vector3.
		/// [JP] Jolt のベクトルをエンジンの Vector3 へ変換する。
		JPH::Vec3 gravity = joltManager_.PhysicsSystem().GetGravity();
		return Vector3(gravity.GetX(), gravity.GetY(), gravity.GetZ());
	}

	/**
	* [EN]
	* Creates and activates a rigid body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 剛体を生成して有効化する。
	*/
	ScBodyID Physics::CreateRigidbody(const RigidbodyDesc& desc)
	{
		/// [EN] A handle that no longer points at a pooled shape gives an invalid BodyID.
		/// [JP] プールの形状を指さなくなったハンドルなら、無効な BodyID を返す。
		JPH::ShapeRefC shape = joltManager_.ShapePool().Get(desc.shape_);
		if (!shape)
		{
			return ScInvalidBodyID;
		}

		/// [EN] Shape, initial pose, motion type and the packed object layer.
		/// [JP] 形状、初期姿勢、運動タイプ、パック済みのオブジェクトレイヤー。
		JPH::BodyCreationSettings settings(shape, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), static_cast<JPH::EMotionType>(desc.motionType_), desc.layer_);

		/// [EN] Material and motion properties; the EntityID is stored bit for bit as user data.
		/// [JP] 材質と運動の特性。EntityID はビットのままユーザーデータに格納する。
		settings.mAllowedDOFs = static_cast<JPH::EAllowedDOFs>(desc.allowedDOFs_);
		settings.mLinearDamping = desc.linearDamping_;
		settings.mAngularDamping = desc.angularDamping_;
		settings.mFriction = desc.friction_;
		settings.mRestitution = desc.restitution_;
		settings.mGravityFactor = desc.gravityFactor_;
		settings.mUserData = std::bit_cast<JPH::uint64>(desc.userData_);
		settings.mIsSensor = desc.isSensor_;

		/// [EN] Jolt allocates motion properties only at creation, so a body whose motion type may change keeps them even while static.
		/// [JP] Jolt は運動特性を生成時にしか確保しないので、運動タイプが変わりうるボディはスタティックの間も持たせておく。
		settings.mAllowDynamicOrKinematic = desc.allowMotionTypeChange_;

		/// [EN] A kinematic trigger needs this to detect static bodies; other kinematic pairs have no simulation effect, so the flag stays off for them.
		/// [JP] キネマティックのトリガーがスタティックのボディを検知するのに必要。それ以外のキネマティックの組はシミュレーションに影響しないので立てない。
		settings.mCollideKinematicVsNonDynamic = desc.motionType_ == ScMotionType::Kinematic && desc.isSensor_;

		/// [EN] A kinematic body that goes to sleep loses all its contacts, so it would leave every trigger it rests in; kinematic bodies are few, so keeping them awake is cheap.
		/// [JP] 眠ったキネマティックのボディは接触をすべて失い、止まっているトリガーから出たことになってしまう。キネマティックのボディは数が少ないので、起こしたままでも負荷は小さい。
		settings.mAllowSleeping = desc.motionType_ != ScMotionType::Kinematic;

		/// [EN] Static bodies have no mass or motion quality, unless they may later become dynamic or kinematic.
		/// [JP] 静的ボディは質量も運動品質も持たない。後から動的・キネマティックになりうる場合は別。
		if (desc.motionType_ != ScMotionType::Static || desc.allowMotionTypeChange_)
		{
			/// [EN] The mass is given; the inertia is derived from the shape scaled to that mass.
			/// [JP] 質量は指定値を使い、慣性はその質量に合わせて形状から求める。
			settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			settings.mMassPropertiesOverride.mMass = desc.mass_;

			/// [EN] Continuous collision sweeps the body along its motion so fast bodies do not tunnel.
			/// [JP] 連続衝突判定は移動経路に沿ってボディを掃引し、高速なボディのすり抜けを防ぐ。
			settings.mMotionQuality = desc.continuousCollision_ ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
		}

		/// [EN] Creation fails when Jolt has run out of body slots.
		/// [JP] Jolt のボディ枠が尽きていると生成に失敗する。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::Body* body = bodyInterface.CreateBody(settings);
		if (!body)
		{
			return ScInvalidBodyID;
		}

		/// [EN] Adds the body to the simulation, awake.
		/// [JP] ボディを起きた状態でシミュレーションへ加える。
		bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
		return body->GetID().GetIndexAndSequenceNumber();
	}

	/**
	* [EN]
	* Creates and activates a soft body from mesh data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メッシュデータからソフトボディを生成して有効化する。
	*/
	ScBodyID Physics::CreateSoftbody(const SoftbodyDesc& desc)
	{
		/// [EN] Mesh topology and constraints, shareable between soft bodies built from the same mesh.
		/// [JP] メッシュの構造と拘束。同じメッシュから作るソフトボディ間で共有できる。
		JPH::Ref<JPH::SoftBodySharedSettings> sharedSettings = new JPH::SoftBodySharedSettings();

		/// [EN] Every mesh position becomes a simulated vertex.
		/// [JP] メッシュの各位置を、シミュレーションされる頂点にする。
		sharedSettings->mVertices.reserve(desc.positions_.size());
		for (const Vector3& position : desc.positions_)
		{
			sharedSettings->mVertices.push_back(JPH::SoftBodySharedSettings::Vertex(JPH::Float3(position.x, position.y, position.z)));
		}

		/// [EN] Upper bound for the vertex indices of each face.
		/// [JP] 各面の頂点インデックスの上限。
		Uint32 vertexCount = static_cast<Uint32>(desc.positions_.size());

		/// [EN] Every three indices form one triangle face; a trailing partial triangle is ignored.
		/// [JP] インデックス3つで三角形の面1つ。末尾の3つに満たない分は無視する。
		sharedSettings->mFaces.reserve(desc.indices_.size() / 3);
		for (Size faceIndex = 0; faceIndex + 2 < desc.indices_.size(); faceIndex += 3)
		{
			Uint32 vertex0 = desc.indices_[faceIndex];
			Uint32 vertex1 = desc.indices_[faceIndex + 1];
			Uint32 vertex2 = desc.indices_[faceIndex + 2];

			/// [EN] A face referencing a vertex that does not exist is skipped.
			/// [JP] 存在しない頂点を参照する面は飛ばす。
			if (vertex0 >= vertexCount || vertex1 >= vertexCount || vertex2 >= vertexCount)
			{
				SC_LOG_WARNING("Softbody: 頂点数({})を超える頂点インデックス({}, {}, {})を持つ面をスキップしました", vertexCount, vertex0, vertex1, vertex2);
				continue;
			}

			/// [EN] A face that repeats a vertex has no area and is skipped.
			/// [JP] 同じ頂点を重複して使う面は面積を持たないので飛ばす。
			JPH::SoftBodySharedSettings::Face face(vertex0, vertex1, vertex2);
			if (face.IsDegenerate())
			{
				continue;
			}

			sharedSettings->AddFace(face);
		}

		/// [EN] Without a single valid face there is nothing to simulate.
		/// [JP] 有効な面が1つも無ければ、シミュレーションするものが無い。
		if (sharedSettings->mFaces.empty())
		{
			return ScInvalidBodyID;
		}

		/// [EN] Edge, shear and bend constraints from the faces; compliance 0 is rigid, higher is softer.
		/// [JP] 面から辺・せん断・曲げの拘束を作る。コンプライアンスは 0 で硬く、大きいほど柔らかい。
		JPH::SoftBodySharedSettings::VertexAttributes vertexAttributes(desc.edgeCompliance_, desc.shearCompliance_, desc.bendCompliance_);
		sharedSettings->CreateConstraints(&vertexAttributes, 1);

		/// [EN] Reorders the constraints into groups that can be solved in parallel.
		/// [JP] 拘束を、並列に解けるグループへ並べ替える。
		sharedSettings->Optimize();

		/// [EN] Pose, layer and solver/material properties; the EntityID is stored as user data.
		/// [JP] 姿勢、レイヤー、ソルバと材質の特性。EntityID はユーザーデータに格納する。
		JPH::SoftBodyCreationSettings settings(sharedSettings, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), desc.layer_);
		settings.mNumIterations = desc.numIterations_;
		settings.mLinearDamping = desc.linearDamping_;
		settings.mPressure = desc.pressure_;
		settings.mFriction = desc.friction_;
		settings.mRestitution = desc.restitution_;
		settings.mGravityFactor = desc.gravityFactor_;
		settings.mUserData = std::bit_cast<JPH::uint64>(desc.userData_);

		/// [EN] Creates the body and adds it to the simulation, awake.
		/// [JP] ボディを生成し、起きた状態でシミュレーションへ加える。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		return bodyInterface.CreateAndAddSoftBody(settings, JPH::EActivation::Activate).GetIndexAndSequenceNumber();
	}

	/**
	* [EN]
	* Removes a body from simulation without destroying it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディを破棄せずシミュレーションから外す。
	*/
	void Physics::SuspendBody(ScBodyID scBodyID)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] Removing keeps the body and its state; it only stops simulating and colliding.
		/// [JP] 外してもボディと状態は残り、シミュレーションと衝突だけが止まる。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.IsAdded(bodyID))
		{
			bodyInterface.RemoveBody(bodyID);
		}
	}

	/**
	* [EN]
	* Adds a suspended body back to simulation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 停止中のボディをシミュレーションへ戻す。
	*/
	void Physics::ResumeBody(ScBodyID scBodyID)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] Re-adds the body awake; a body already in the simulation is left alone.
		/// [JP] ボディを起きた状態で戻す。既にシミュレーション中のボディはそのままにする。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (!bodyInterface.IsAdded(bodyID))
		{
			bodyInterface.AddBody(bodyID, JPH::EActivation::Activate);
		}
	}

	/**
	* [EN]
	* Removes and destroys a physics body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理ボディを削除して破棄する。
	*/
	void Physics::DestroyBody(ScBodyID scBodyID)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] A suspended body is already out of the simulation; only an added one needs removing.
		/// [JP] 停止中のボディは既にシミュレーション外なので、追加済みのものだけ外す。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.IsAdded(bodyID))
		{
			bodyInterface.RemoveBody(bodyID);
		}
		bodyInterface.DestroyBody(bodyID);
	}

	/**
	* [EN]
	* Replaces the collision shape assigned to a body. A dynamic body
	* keeps its mass; only its inertia follows the new shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディに割り当てられた衝突形状を置き換える。動的ボディは質量を保ち、
	* 慣性だけを新しい形状に合わせる。
	*/
	void Physics::BodyShape(ScBodyID scBodyID, ScShapeHandle shape)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] A handle that no longer points at a pooled shape leaves the body unchanged.
		/// [JP] プールの形状を指さなくなったハンドルなら、ボディはそのままにする。
		JPH::ShapeRefC newShape = joltManager_.ShapePool().Get(shape);
		if (!newShape)
		{
			return;
		}

		/// [EN] Swaps the shape and wakes the body; Jolt's own recompute would replace the mass with shape volume times density.
		/// [JP] 形状を差し替えてボディを起こす。Jolt の再計算に任せると、質量が形状の体積×密度に置き換わる。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		bodyInterface.SetShape(bodyID, newShape, false, JPH::EActivation::Activate);

		/// [EN] Mass properties are edited on the body directly, so it is write-locked.
		/// [JP] 質量特性はボディを直接書き換えるので、書き込みロックをかける。
		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), bodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		/// [EN] Every body with motion properties keeps its mass up to date, so one that becomes dynamic later already has the right inertia.
		/// [JP] 運動特性を持つボディは全て質量を最新に保ち、後から動的になったときに正しい慣性を持っているようにする。
		JPH::Body& body = lock.GetBody();
		JPH::MotionProperties* motionProperties = body.GetMotionPropertiesUnchecked();
		if (!motionProperties)
		{
			return;
		}

		/// [EN] With every translation axis locked the inverse mass is 0 and the mass cannot be read back.
		/// [JP] 移動軸が全て固定されていると逆質量は 0 になり、質量を読み戻せない。
		Float inverseMass = motionProperties->GetInverseMassUnchecked();
		if (inverseMass <= 0.0f)
		{
			return;
		}

		/// [EN] Keeps the current mass and derives the inertia from the new shape, as at creation.
		/// [JP] 生成時と同じく、質量は今の値を保ち、慣性は新しい形状から求める。
		JPH::MassProperties massProperties = newShape->GetMassProperties();
		massProperties.ScaleToMass(1.0f / inverseMass);
		motionProperties->SetMassProperties(motionProperties->GetAllowedDOFs(), massProperties);
	}

	/**
	* [EN]
	* Returns the world transform of a body: its rotation followed by its
	* translation, with no scale. Read the position with Translation()
	* and the rotation with Quaternion::CreateFromRotationMatrix().
	* Returns the identity for an invalid ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディのワールド変換を返す。回転の後に平行移動を行う行列で、スケールは
	* 含まない。位置は Translation()、回転は
	* Quaternion::CreateFromRotationMatrix() で取り出す。無効な ID では
	* 単位行列を返す。
	*/
	Matrix Physics::BodyTransform(ScBodyID scBodyID)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return Matrix::Identity;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 position = bodyInterface.GetPosition(bodyID);
		JPH::Quat rotation = bodyInterface.GetRotation(bodyID);

		/// [EN] Built from Jolt's position and quaternion in the engine's row-vector order, rotation first and translation second, rather than by transposing Jolt's column-major matrix.
		/// [JP] Jolt の行列を転置するのではなく、Jolt の位置とクォータニオンから、エンジンの行ベクトルの順（先に回転、後に平行移動）で組み立てる。
		return Matrix::CreateFromQuaternion(Quaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW())) * Matrix::CreateTranslation(Vector3(position.GetX(), position.GetY(), position.GetZ()));
	}

	/**
	* [EN]
	* Moves a body to a world position and rotation directly, without
	* simulating the motion in between, and without waking it. Does
	* nothing when the body is already there.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディを、間の動きをシミュレーションせずにワールドの位置と回転へ直接
	* 移す。ボディは起こさない。既にその位置と回転にあれば何もしない。
	*/
	void Physics::BodyTransform(ScBodyID scBodyID, const Vector3& position, const Quaternion& rotation)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] The "when changed" variant skips the broad-phase update for a body that has not moved, so calling this every frame for still bodies costs almost nothing.
		/// [JP] 「変わったときだけ」版は、動いていないボディのブロードフェーズ更新を省くため、止まっているボディに毎フレーム呼んでもほとんどコストがかからない。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		bodyInterface.SetPositionAndRotationWhenChanged(bodyID, JPH::RVec3(position.x, position.y, position.z), JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w).Normalized(), JPH::EActivation::DontActivate);
	}

	/**
	* [EN]
	* Returns the linear velocity (m/s) of a body; zero for a static
	* body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディの速度(m/s)を返す。静的ボディでは 0。
	*/
	Vector3 Physics::BodyLinearVelocity(ScBodyID scBodyID)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		JPH::Vec3 linearVelocity = joltManager_.BodyInterface().GetLinearVelocity(bodyID);
		return Vector3(linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ());
	}

	/**
	* [EN]
	* Replaces the linear velocity (m/s) of a dynamic or kinematic body
	* and wakes it when the velocity is not zero. A kinematic body keeps
	* the velocity until it is replaced. Has no effect on a static body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 動的またはキネマティックのボディの速度(m/s)を置き換え、速度が 0 で
	* なければボディを起こす。キネマティックのボディは、置き換えられるまで
	* その速度を保つ。静的ボディには効かない。
	*/
	void Physics::BodyLinearVelocity(ScBodyID scBodyID, const Vector3& linearVelocity)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] The velocity is clamped to the body's maximum speed, and frozen axes stay at zero.
		/// [JP] 速度はボディの最大速度に収められ、固定した軸は 0 のまま。
		joltManager_.BodyInterface().SetLinearVelocity(bodyID, JPH::Vec3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
	}

	/**
	* [EN]
	* Switches a rigid body between Dynamic, Kinematic and Static: changes
	* its motion type, the motion-type bits of its object layer, whether
	* it may sleep, and for a kinematic trigger the flag that lets it
	* meet static bodies, so it ends up as if it had been created that
	* way. A body becoming kinematic starts at rest. Leaving Static needs
	* a body created with allowMotionTypeChange_; soft bodies are left
	* as they are.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 剛体を Dynamic・Kinematic・Static の間で切り替える。運動タイプ、
	* オブジェクトレイヤーの運動タイプのビット、眠れるかどうか、キネマ
	* ティックのトリガーではスタティックのボディと組になるためのフラグを
	* 変え、最初からその設定で作ったのと同じ状態にする。キネマティックに
	* なるボディは静止した状態から始める。Static から抜けるには
	* allowMotionTypeChange_ を付けて作ったボディが要る。ソフトボディは
	* そのまま。
	*/
	void Physics::BodyMotionType(ScBodyID scBodyID, ScMotionType motionType)
	{
		const JPH::BodyID bodyID(scBodyID);
		const JPH::EMotionType joltMotionType = static_cast<JPH::EMotionType>(motionType);
		if (bodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();

		/// [EN] A soft body is always dynamic, and a body without motion properties can never leave Static.
		/// [JP] ソフトボディは常に動的で、運動特性の無いボディは Static から抜けられない。
		/// [EN] The scope releases the read lock before the calls below, which lock the body themselves.
		/// [JP] 下の呼び出しは自分でボディをロックするので、その前にこのスコープで読み込みロックを外す。
		{
			JPH::BodyLockRead lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), bodyID);
			if (!lock.Succeeded())
			{
				return;
			}

			const JPH::Body& body = lock.GetBody();
			if (body.IsSoftBody())
			{
				return;
			}

			if (joltMotionType != JPH::EMotionType::Static && !body.GetMotionPropertiesUnchecked())
			{
				SC_LOG_WARNING("Physics::BodyMotionType: Static で生成されたボディは Dynamic/Kinematic に切り替えられません (allowMotionTypeChange_ が必要です)");
				return;
			}
		}

		/// [EN] Jolt deactivates a body that becomes static and wakes one that starts moving.
		/// [JP] Jolt はスタティックになるボディを止め、動き出すボディを起こす。
		bodyInterface.SetMotionType(bodyID, joltMotionType, JPH::EActivation::Activate);

		/// [EN] A kinematic body keeps moving at its velocity, so it starts at rest as one created that way would.
		/// [JP] キネマティックのボディは自分の速度で動き続けるので、最初からそう作ったときと同じく静止から始める。
		if (joltMotionType == JPH::EMotionType::Kinematic)
		{
			bodyInterface.SetLinearAndAngularVelocity(bodyID, JPH::Vec3::sZero(), JPH::Vec3::sZero());
		}

		/// [EN] Only the motion-type bits change; the actor's layer and the SENSOR and PLANAR bits stay, and the body moves to the matching broad-phase layer.
		/// [JP] 変えるのは運動タイプのビットだけで、Actor のレイヤーと SENSOR・PLANAR ビットはそのまま。ボディは対応するブロードフェーズレイヤーへ移る。
		JPH::ObjectLayer motionLayer = Layers::DYNAMIC;
		switch (joltMotionType)
		{
		case JPH::EMotionType::Kinematic:
			motionLayer = Layers::KINEMATIC;
			break;
		case JPH::EMotionType::Static:
			motionLayer = Layers::STATIC;
			break;
		default:
			break;
		}

		JPH::ObjectLayer layer = bodyInterface.GetObjectLayer(bodyID);
		layer = static_cast<JPH::ObjectLayer>((layer & ~((1u << Layers::MOTION_TYPE_BITS) - 1u)) | motionLayer);
		bodyInterface.SetObjectLayer(bodyID, layer);

		/// [EN] The flags decided at creation follow the new motion type; the body interface has no setters for them, so they are written on the locked body.
		/// [JP] 生成時に決めたフラグを新しい運動タイプに合わせる。ボディインターフェースに設定関数が無いので、ロックしたボディに書く。
		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), bodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		JPH::Body& body = lock.GetBody();
		body.SetCollideKinematicVsNonDynamic(joltMotionType == JPH::EMotionType::Kinematic && body.IsSensor());

		/// [EN] As at creation, a kinematic body stays awake so it does not leave the triggers it rests in.
		/// [JP] 生成時と同じく、キネマティックのボディは止まっているトリガーから出たことにならないよう起こしたままにする。
		if (body.GetMotionPropertiesUnchecked())
		{
			body.SetAllowSleeping(joltMotionType != JPH::EMotionType::Kinematic);
		}
	}

	/**
	* [EN]
	* Returns the world-space angular velocity (rad/s) of a body; zero
	* for a static body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディのワールド空間の角速度(rad/s)を返す。静的ボディでは 0。
	*/
	Vector3 Physics::BodyAngularVelocity(ScBodyID scBodyID)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		JPH::Vec3 angularVelocity = joltManager_.BodyInterface().GetAngularVelocity(bodyID);
		return Vector3(angularVelocity.GetX(), angularVelocity.GetY(), angularVelocity.GetZ());
	}

	/**
	* [EN]
	* Replaces the world-space angular velocity (rad/s) of a dynamic or
	* kinematic body and wakes it when the angular velocity is not zero.
	* A kinematic body keeps the angular velocity until it is replaced.
	* Has no effect on a static body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 動的またはキネマティックのボディのワールド空間の角速度(rad/s)を
	* 置き換え、角速度が 0 でなければボディを起こす。キネマティックの
	* ボディは、置き換えられるまでその角速度を保つ。静的ボディには効かない。
	*/
	void Physics::BodyAngularVelocity(ScBodyID scBodyID, const Vector3& angularVelocity)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] The angular velocity is clamped to the body's maximum angular speed, and frozen rotation axes stay at zero.
		/// [JP] 角速度はボディの最大角速度に収められ、固定した回転軸は 0 のまま。
		joltManager_.BodyInterface().SetAngularVelocity(bodyID, JPH::Vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
	}

	/**
	* [EN]
	* Turns a body into a trigger or back into a solid body: switches its
	* sensor flag, the SENSOR bit of its object layer, and for a
	* kinematic body the flag that lets it meet static bodies, so it
	* ends up as if it had been created that way.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディをトリガーにする、または実体のあるボディへ戻す。センサーの
	* フラグ、オブジェクトレイヤーの SENSOR ビット、キネマティックの
	* ボディではスタティックのボディと組になるためのフラグを切り替え、
	* 最初からその設定で作ったのと同じ状態にする。
	*/
	void Physics::BodyTrigger(ScBodyID scBodyID, Bool isTrigger)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();

		/// [EN] The sensor flag decides whether the body pushes back; a trigger only reports overlaps.
		/// [JP] センサーのフラグが、ボディが押し返すかを決める。トリガーは重なりを知らせるだけ。
		bodyInterface.SetIsSensor(bodyID, isTrigger);

		/// [EN] Only the SENSOR bit changes; the motion type, the actor's layer and the PLANAR bit stay as they are, and the body moves to the matching broad-phase layer.
		/// [JP] 変えるのは SENSOR ビットだけで、運動タイプ・Actor のレイヤー・PLANAR ビットはそのまま。ボディは対応するブロードフェーズレイヤーへ移る。
		JPH::ObjectLayer layer = bodyInterface.GetObjectLayer(bodyID);
		if (isTrigger)
		{
			layer |= Layers::SENSOR;
		}
		else
		{
			layer = static_cast<JPH::ObjectLayer>(layer & ~Layers::SENSOR);
		}
		bodyInterface.SetObjectLayer(bodyID, layer);

		/// [EN] As at creation, only a kinematic trigger needs to meet static bodies.
		/// [JP] 生成時と同じく、スタティックのボディと組になる必要があるのはキネマティックのトリガーだけ。
		Bool collideKinematicVsNonDynamic = bodyInterface.GetMotionType(bodyID) == JPH::EMotionType::Kinematic && isTrigger;

		/// [EN] The body interface has no setter for this flag, so it is written on the locked body; the lock is taken after the calls above, which lock the body themselves.
		/// [JP] このフラグはボディインターフェースに設定関数が無いので、ロックしたボディに書く。上の呼び出しは自分でボディをロックするので、ロックはその後で取る。
		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), bodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		lock.GetBody().SetCollideKinematicVsNonDynamic(collideKinematicVsNonDynamic);
	}

	/**
	* [EN]
	* Reports whether a body is currently added to the simulation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディが現在シミュレーションに追加されているかを返す。
	*/
	Bool Physics::BodyExists(ScBodyID scBodyID)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return false;
		}

		/// [EN] Destroyed and suspended bodies are both outside the simulation and report false.
		/// [JP] 破棄済みのボディも停止中のボディもシミュレーション外なので false を返す。
		return joltManager_.BodyInterface().IsAdded(bodyID);
	}

	/**
	* [EN]
	* Writes the world positions of all soft-body vertices.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソフトボディの全頂点のワールド位置を書き出す。
	*/
	void Physics::BodyVertex(ScBodyID scBodyID, DynamicArray<Vector3>& outPositions)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] Vertices are read directly from the body, so it is read-locked while copying.
		/// [JP] 頂点はボディから直接読むので、コピーの間は読み込みロックをかける。
		const JPH::BodyLockInterface& lockInterface = joltManager_.PhysicsSystem().GetBodyLockInterface();

		JPH::BodyLockRead lock(lockInterface, bodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		/// [EN] A soft body keeps its vertices in its motion properties.
		/// [JP] ソフトボディは頂点を運動特性の中に持つ。
		const JPH::Body& body = lock.GetBody();
		const JPH::SoftBodyMotionProperties* motionProperties = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionPropertiesUnchecked());
		if (!motionProperties)
		{
			return;
		}

		/// [EN] Vertex positions are relative to the center of mass, so this transform takes them to world space.
		/// [JP] 頂点位置は重心からの相対なので、この変換でワールド空間へ移す。
		JPH::RMat44 transform = body.GetCenterOfMassTransform();
		const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& vertices = motionProperties->GetVertices();

		/// [EN] Replaces the previous contents of outPositions.
		/// [JP] outPositions の以前の中身は置き換える。
		outPositions.clear();
		outPositions.reserve(vertices.size());
		for (const JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
		{
			JPH::RVec3 worldPosition = transform * vertex.mPosition;
			outPositions.push_back(Vector3(static_cast<Float>(worldPosition.GetX()), static_cast<Float>(worldPosition.GetY()), static_cast<Float>(worldPosition.GetZ())));
		}
	}

	/**
	* [EN]
	* Returns the entity identifier stored in a body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディに格納されたエンティティ識別子を返す。
	*/
	EntityID Physics::BodyEntityID(ScBodyID scBodyID)const
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return EntityID{};
		}

		/// [EN] A body that cannot be locked (already destroyed) yields a null EntityID.
		/// [JP] ロックできない(破棄済みの)ボディなら、空の EntityID を返す。
		const JPH::BodyLockInterface& lockInterface = joltManager_.PhysicsSystem().GetBodyLockInterface();

		JPH::BodyLockRead lock(lockInterface, bodyID);
		if (!lock.Succeeded())
		{
			return EntityID{};
		}

		/// [EN] The user data holds the EntityID bit for bit, as written at creation.
		/// [JP] ユーザーデータには、生成時に書いた EntityID がビットのまま入っている。
		return std::bit_cast<EntityID>(static_cast<Uint64>(lock.GetBody().GetUserData()));
	}

	/**
	* [EN]
	* Returns the identifiers of all physics bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべての物理ボディ識別子を返す。
	*/
	DynamicArray<ScBodyID> Physics::BodyList()const
	{
		/// [EN] Includes suspended bodies, since they still exist in Jolt.
		/// [JP] 停止中のボディも Jolt 上には存在するので含まれる。
		JPH::BodyIDVector bodyIDs;
		joltManager_.PhysicsSystem().GetBodies(bodyIDs);
		DynamicArray<ScBodyID> result;
		result.reserve(bodyIDs.size());
		for (const JPH::BodyID bodyID : bodyIDs)
		{
			result.push_back(bodyID.GetIndexAndSequenceNumber());
		}
		return result;
	}

	/**
	* [EN]
	* Adds a force (N) at the body's center of mass for the next step
	* and wakes the body. On a soft body the force is spread evenly
	* over its vertices. Non-dynamic bodies are left untouched.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次のステップの間、ボディの重心に力(N)を加え、ボディを起こす。
	* ソフトボディでは力を頂点へ均等に分ける。動的でないボディには
	* 何もしない。
	*/
	void Physics::AddForce(ScBodyID scBodyID, const Vector3& force)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] Jolt accumulates the force until the next update and skips bodies that are not dynamic.
		/// [JP] Jolt は次の更新まで力を溜め、動的でないボディは飛ばす。
		/// [EN] A soft body consumes the same accumulated force, dividing it among its vertices.
		/// [JP] ソフトボディも同じ溜めた力を使い、頂点の数で割って配る。
		joltManager_.BodyInterface().AddForce(bodyID, JPH::Vec3(force.x, force.y, force.z));
	}

	/**
	* [EN]
	* Adds an impulse (kg·m/s) at the body's center of mass, changing
	* its velocity at once, and wakes the body. On a soft body every
	* movable vertex gets the same change of velocity. Non-dynamic
	* bodies are left untouched.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディの重心に力積(kg·m/s)を加えて速度を一度に変え、ボディを
	* 起こす。ソフトボディでは、動ける頂点すべてに同じ速度変化を与える。
	* 動的でないボディには何もしない。
	*/
	void Physics::AddImpulse(ScBodyID scBodyID, const Vector3& impulse)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] A rigid body takes the impulse directly as a change of its linear velocity.
		/// [JP] 剛体は力積をそのまま移動速度の変化として受け取る。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.GetBodyType(bodyID) != JPH::EBodyType::SoftBody)
		{
			bodyInterface.AddImpulse(bodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
			return;
		}

		/// [EN] A soft body moves by its vertices' own velocities, so the impulse is applied to them directly.
		/// [JP] ソフトボディは各頂点の速度で動くので、力積を頂点へ直接与える。
		/// [EN] The scope releases the write lock before the body is woken, since waking takes its own lock.
		/// [JP] ボディを起こす処理は自分でロックを取るので、その前にこのスコープで書き込みロックを外す。
		{
			JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), bodyID);
			if (!lock.Succeeded())
			{
				return;
			}

			/// [EN] A soft body keeps its vertices in its motion properties.
			/// [JP] ソフトボディは頂点を運動特性の中に持つ。
			JPH::SoftBodyMotionProperties* motionProperties = static_cast<JPH::SoftBodyMotionProperties*>(lock.GetBody().GetMotionPropertiesUnchecked());
			if (!motionProperties)
			{
				return;
			}

			/// [EN] Total mass of the movable vertices; a vertex with inverse mass 0 is pinned and takes no share.
			/// [JP] 動ける頂点の質量の合計。逆質量が 0 の頂点は固定されていて、分け前を受け取らない。
			JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& vertices = motionProperties->GetVertices();
			Float totalMass = 0.0f;
			for (const JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
			{
				if (vertex.mInvMass > 0.0f)
				{
					totalMass += 1.0f / vertex.mInvMass;
				}
			}

			/// [EN] With every vertex pinned the body cannot move.
			/// [JP] 全ての頂点が固定されていれば、ボディは動けない。
			if (totalMass <= 0.0f)
			{
				return;
			}

			/// [EN] Impulse over total mass is the change of velocity; giving every movable vertex the same change moves the whole body without deforming it.
			/// [JP] 力積を総質量で割ったものが速度変化。動ける頂点すべてに同じ変化を与えれば、形を崩さずボディ全体が動く。
			JPH::Vec3 deltaVelocity = JPH::Vec3(impulse.x, impulse.y, impulse.z) / totalMass;
			for (JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
			{
				if (vertex.mInvMass > 0.0f)
				{
					vertex.mVelocity += deltaVelocity;
				}
			}
		}

		/// [EN] A sleeping body would ignore the new velocities, so it is woken.
		/// [JP] 眠っているボディは新しい速度を無視するので、起こす。
		bodyInterface.ActivateBody(bodyID);
	}

	/**
	* [EN]
	* Adds a torque (N·m) in world space for the next step and wakes
	* the body. Has no effect on a soft body or a non-dynamic body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次のステップの間、ワールド空間のトルク(N·m)を加え、ボディを起こす。
	* ソフトボディと動的でないボディには効かない。
	*/
	void Physics::AddTorque(ScBodyID scBodyID, const Vector3& torque)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] Jolt accumulates the torque until the next update and skips bodies that are not dynamic.
		/// [JP] Jolt は次の更新までトルクを溜め、動的でないボディは飛ばす。
		joltManager_.BodyInterface().AddTorque(bodyID, JPH::Vec3(torque.x, torque.y, torque.z));
	}

	/**
	* [EN]
	* Adds an angular impulse (N·m·s) in world space, changing the
	* body's angular velocity at once, and wakes the body. Has no
	* effect on a soft body or a non-dynamic body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールド空間の角力積(N·m·s)を加えて角速度を一度に変え、ボディを
	* 起こす。ソフトボディと動的でないボディには効かない。
	*/
	void Physics::AddSpin(ScBodyID scBodyID, const Vector3& angularImpulse)
	{
		const JPH::BodyID bodyID(scBodyID);
		if (bodyID.IsInvalid())
		{
			return;
		}

		/// [EN] The angular velocity changes by the inverse inertia times the angular impulse.
		/// [JP] 角速度は、慣性の逆数に角力積を掛けた分だけ変わる。
		joltManager_.BodyInterface().AddAngularImpulse(bodyID, JPH::Vec3(angularImpulse.x, angularImpulse.y, angularImpulse.z));
	}

	/**
	* [EN]
	* Sets the velocities of a kinematic body so that it reaches the
	* target position and rotation after elapsedTime, and wakes the
	* body. Has no effect on a body that is not kinematic.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* elapsedTime 後に目標の位置と回転へ着くようキネマティックボディの
	* 速度を設定し、ボディを起こす。キネマティックでないボディには効かない。
	*/
	void Physics::MoveTarget(ScBodyID scBodyID, const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime)
	{
		const JPH::BodyID bodyID(scBodyID);
		/// [EN] The velocity is the distance divided by elapsedTime, so a zero step has no defined velocity.
		/// [JP] 速度は距離を elapsedTime で割ったものなので、時間が 0 のステップでは速度が定まらない。
		if (bodyID.IsInvalid() || elapsedTime <= 0.0f)
		{
			return;
		}

		/// [EN] Only a kinematic body follows a target; a dynamic one is driven by forces and a static one never moves.
		/// [JP] 目標に従うのはキネマティックボディだけ。動的ボディは力で動き、静的ボディは動かない。
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.GetMotionType(bodyID) != JPH::EMotionType::Kinematic)
		{
			return;
		}

		/// [EN] Moving by velocity rather than by teleporting lets the body push dynamic bodies and carry whatever stands on it.
		/// [JP] 瞬間移動ではなく速度で動かすことで、ボディが動的ボディを押し、上に乗っているものを運べる。
		bodyInterface.MoveKinematic(bodyID, JPH::RVec3(targetPosition.x, targetPosition.y, targetPosition.z), JPH::Quat(targetRotation.x, targetRotation.y, targetRotation.z, targetRotation.w).Normalized(), elapsedTime);
	}

	/**
	* [EN]
	* Creates a hinge constraint between two bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2つのボディ間にヒンジ拘束を生成する。
	*/
	ScConstraintHandle Physics::CreateHingeJoint(ScBodyID scBodyA, ScBodyID scBodyB, const HingeJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 bodyPosition = bodyInterface.GetPosition(bodyA);
		JPH::Quat bodyRotation = bodyInterface.GetRotation(bodyA);

		/// [EN] The anchor and axis are given in bodyA's local space; both go to world space.
		/// [JP] アンカーと軸は bodyA のローカル空間で与えられるので、どちらもワールド空間へ移す。
		JPH::Vec3 worldPosition(bodyPosition.GetX(), bodyPosition.GetY(), bodyPosition.GetZ());
		JPH::Vec3 worldAnchor = worldPosition + bodyRotation * JPH::Vec3(desc.anchor_.x, desc.anchor_.y, desc.anchor_.z);

		/// [EN] A zero-length axis falls back to world up.
		/// [JP] 長さ 0 の軸はワールドの上方向で代用する。
		JPH::Vec3 worldAxis = (bodyRotation * JPH::Vec3(desc.axis_.x, desc.axis_.y, desc.axis_.z)).NormalizedOr(JPH::Vec3::sAxisY());

		/// [EN] Any direction perpendicular to the axis; it marks angle 0 for the limits.
		/// [JP] 軸に垂直な任意の方向。角度制限における 0 度の基準になる。
		JPH::Vec3 worldNormal = worldAxis.GetNormalizedPerpendicular();

		/// [EN] Both bodies share the same pivot and axes, so the current pose is angle 0.
		/// [JP] 両ボディが同じ支点と軸を共有するので、現在の姿勢が 0 度になる。
		JPH::HingeConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = settings.mPoint2 = JPH::RVec3(worldAnchor);
		settings.mHingeAxis1 = settings.mHingeAxis2 = worldAxis;
		settings.mNormalAxis1 = settings.mNormalAxis2 = worldNormal;

		/// [EN] Limits are authored in degrees; Jolt takes radians.
		/// [JP] 制限は度で指定し、Jolt にはラジアンで渡す。
		if (desc.useLimits_)
		{
			settings.mLimitsMin = ToRadians(desc.minAngle_);
			settings.mLimitsMax = ToRadians(desc.maxAngle_);
		}

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [EN]
	* Creates a fixed constraint between two bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2つのボディ間に固定拘束を生成する。
	*/
	ScConstraintHandle Physics::CreateFixedJoint(ScBodyID scBodyA, ScBodyID scBodyB, const FixedJointDesc&)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		/// [EN] Locks the bodies in their current relative pose; the pivot is detected from their positions.
		/// [JP] 両ボディを現在の相対姿勢のまま固定する。支点は両者の位置から自動で求める。
		JPH::FixedConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = true;

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [EN]
	* Creates a spring distance constraint between two bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2つのボディ間にばね距離拘束を生成する。
	*/
	ScConstraintHandle Physics::CreateSpringJoint(ScBodyID scBodyA, ScBodyID scBodyB, const SpringJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 positionA = bodyInterface.GetPosition(bodyA);
		JPH::Quat rotationA = bodyInterface.GetRotation(bodyA);

		/// [EN] bodyA's end of the spring: the anchor, given in bodyA's local space, moved to world space.
		/// [JP] bodyA 側のばねの端: bodyA のローカル空間で与えたアンカーをワールド空間へ移したもの。
		JPH::Vec3 worldPositionA(positionA.GetX(), positionA.GetY(), positionA.GetZ());
		JPH::Vec3 anchorSelf = worldPositionA + rotationA * JPH::Vec3(desc.anchor_.x, desc.anchor_.y, desc.anchor_.z);

		/// [EN] The other end is bodyB's origin, or the anchor itself when attached to the world.
		/// [JP] もう一方の端は bodyB の原点。ワールドに繋ぐ場合はアンカーそのもの。
		JPH::Vec3 anchorOther = anchorSelf;
		if (!bodyB.IsInvalid())
		{
			JPH::RVec3 positionB = bodyInterface.GetPosition(bodyB);
			anchorOther = JPH::Vec3(positionB.GetX(), positionB.GetY(), positionB.GetZ());
		}

		/// [EN] Point1 belongs to the other body and Point2 to bodyA, matching CreateConstraint's body order.
		/// [JP] Point1 は相手側、Point2 は bodyA 側。CreateConstraint のボディ順に合わせている。
		JPH::DistanceConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = JPH::RVec3(anchorOther);
		settings.mPoint2 = JPH::RVec3(anchorSelf);

		/// [EN] Both distances 0 means "unset": a negative value makes Jolt use the current distance.
		/// [JP] 両方 0 は未指定の扱い。負の値を渡すと、Jolt は現在の距離を使う。
		if (desc.minDistance_ == 0.0f && desc.maxDistance_ == 0.0f)
		{
			settings.mMinDistance = -1.0f;
			settings.mMaxDistance = -1.0f;
		}
		else
		{
			settings.mMinDistance = desc.minDistance_;
			settings.mMaxDistance = desc.maxDistance_;
		}

		/// [EN] The distance limits are soft, pulling back like a spring with this frequency and damping.
		/// [JP] 距離の制限は柔らかく、この振動数と減衰のばねとして引き戻す。
		settings.mLimitsSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, desc.frequency_, desc.damping_);

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [EN]
	* Creates a slider constraint between two bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2つのボディ間にスライダー拘束を生成する。
	*/
	ScConstraintHandle Physics::CreateSliderJoint(ScBodyID scBodyA, ScBodyID scBodyB, const SliderJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		/// [EN] The axis is given in bodyA's local space; a zero-length axis falls back to world X.
		/// [JP] 軸は bodyA のローカル空間で与える。長さ 0 の軸はワールドの X 方向で代用する。
		JPH::Quat rotationA = bodyInterface.GetRotation(bodyA);
		JPH::Vec3 worldAxis = (rotationA * JPH::Vec3(desc.axis_.x, desc.axis_.y, desc.axis_.z)).NormalizedOr(JPH::Vec3::sAxisX());

		/// [EN] The bodies may only translate along the axis; the current offset is position 0.
		/// [JP] ボディは軸方向にだけ移動できる。現在のずれが位置 0 になる。
		JPH::SliderConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = true;
		settings.SetSliderAxis(worldAxis);

		/// [EN] Limits on the translation along the axis, in meters.
		/// [JP] 軸方向の移動量の制限(メートル)。
		if (desc.useLimits_)
		{
			settings.mLimitsMin = desc.minDistance_;
			settings.mLimitsMax = desc.maxDistance_;
		}

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [EN]
	* Releases a joint constraint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョイント拘束を解放する。
	*/
	void Physics::DestroyJoint(ScConstraintHandle handle)
	{
		/// [EN] The pool removes the constraint from the simulation and frees its slot; stale handles are ignored.
		/// [JP] プールが拘束をシミュレーションから外してスロットを空ける。古いハンドルは無視される。
		joltManager_.ConstraintPool().Release(joltManager_.PhysicsSystem(), handle);
	}

	/**
	* [EN]
	* Enables each joint whose bodies are both in the simulation and
	* disables the rest, so suspended bodies do not drag their joints.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 両方のボディがシミュレーション中のジョイントを有効にし、それ以外を
	* 無効にする。停止中のボディにジョイントが引きずられないようにする。
	*/
	void Physics::RefleshJoint()
	{
		/// [EN] A world-attached end always counts as present.
		/// [JP] ワールドに繋がった端は、常に存在するものとして扱う。
		joltManager_.ConstraintPool().Refresh();
	}

	/**
	* [EN]
	* Finds the nearest 3D body intersected by a ray.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイと交差する最も近い3Dボディを検索する。
	*/
	Bool Physics::Raycast(const Vector3& origin, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [EN] Only the direction matters; a zero vector is left as is and hits nothing.
		/// [JP] 使うのは向きだけ。ゼロベクトルはそのままにし、何にも当たらない。
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		/// [EN] Jolt's ray is an origin plus a vector whose length is the reach.
		/// [JP] Jolt のレイは、始点と、長さが届く距離になるベクトルで表す。
		JPH::RRayCast ray(JPH::RVec3(origin.x, origin.y, origin.z), dir * maxDistance);

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display: what was asked now, what was found just before returning.
		/// [JP] クエリはすべてデバッグ表示のために記録する。問い合わせた内容はここで、結果は返す直前に入れる。
		QueryDesc query;
		query.kind_ = QueryKind::Raycast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
#endif

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every hit, since the nearest one may be filtered out below.
		/// [JP] 最も近いヒットが下で除外されることもあるので、全てのヒットを集める。
		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [EN] Nearest first, so the first hit that passes the filters is the answer.
		/// [JP] 近い順に並べ、フィルタを通った最初のヒットを答えにする。
		collector.Sort();

		for (const JPH::RayCastResult& hit : collector.mHits)
		{
			/// [EN] 2D canvas bodies share the Jolt world but are not part of 3D queries.
			/// [JP] 2D Canvas のボディは Jolt ワールドを共有するが、3D のクエリには含めない。
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID) & Layers::PLANAR)
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [EN] The hit point lies at the hit fraction along the ray.
			/// [JP] ヒット位置は、レイ上のヒット割合の位置にある。
			JPH::RVec3 hitPosition = ray.GetPointOnRay(hit.mFraction);

			/// [EN] The surface normal needs the body's shape, so the body is read-locked briefly.
			/// [JP] 面の法線はボディの形状から求めるので、短い間だけ読み込みロックをかける。
			JPH::Vec3 normal = JPH::Vec3::sZero();
			{
				JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
				if (lock.Succeeded())
				{
					normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPosition);
				}
			}

			/// [EN] The fraction is a 0-1 ratio of the reach, so it scales back to a distance.
			/// [JP] 割合は届く距離に対する 0～1 の比なので、距離へ戻す。
			outHit.position_ = Vector3(static_cast<Float>(hitPosition.GetX()), static_cast<Float>(hitPosition.GetY()), static_cast<Float>(hitPosition.GetZ()));
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [EN]
	* Finds the nearest 3D body intersected by a swept sphere.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 移動する球と交差する最も近い3Dボディを検索する。
	*/
	Bool Physics::Spherecast(const Vector3& origin, Float radius, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [EN] Only the direction matters; a zero vector is left as is and sweeps nowhere.
		/// [JP] 使うのは向きだけ。ゼロベクトルはそのままにし、どこへも掃引しない。
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		/// [EN] A temporary sphere at origin, swept along dir for maxDistance at unit scale.
		/// [JP] origin に置いた一時的な球を、等倍のまま dir 方向へ maxDistance だけ掃引する。
		JPH::SphereShape sphereShape(radius);
		JPH::RShapeCast shapeCast(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z)), dir * maxDistance);

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display: what was asked now, what was found just before returning.
		/// [JP] クエリはすべてデバッグ表示のために記録する。問い合わせた内容はここで、結果は返す直前に入れる。
		QueryDesc query;
		query.kind_ = QueryKind::Spherecast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
		query.radius_ = radius;
#endif

		JPH::ShapeCastSettings settings;

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every hit, since the nearest one may be filtered out below.
		/// [JP] 最も近いヒットが下で除外されることもあるので、全てのヒットを集める。
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [EN] Nearest first, so the first hit that passes the filters is the answer.
		/// [JP] 近い順に並べ、フィルタを通った最初のヒットを答えにする。
		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			/// [EN] 2D canvas bodies share the Jolt world but are not part of 3D queries.
			/// [JP] 2D Canvas のボディは Jolt ワールドを共有するが、3D のクエリには含めない。
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [EN] The penetration axis points from the sphere into the hit body, so its negation is the surface normal.
			/// [JP] めり込み軸は球から相手のボディへ向くので、その逆向きが面の法線になる。
			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			/// [EN] The contact point is taken on the hit body's surface.
			/// [JP] 接触点は、当たったボディの表面上の点を使う。
			outHit.position_ = Vector3(hit.mContactPointOn2.GetX(), hit.mContactPointOn2.GetY(), hit.mContactPointOn2.GetZ());
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	Bool Physics::Boxcast(const Vector3& origin, const Vector3& halfExtents, const Quaternion& rotation, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [EN] Only the direction matters; a zero vector is left as is and sweeps nowhere.
		/// [JP] 使うのは向きだけ。ゼロベクトルはそのままにし、どこへも掃引しない。
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		/// [EN] A temporary box at origin with the given rotation, swept along dir for maxDistance at unit scale; a zero convex radius keeps it exactly the given size.
		/// [JP] origin に指定の回転で置いた一時的な箱を、等倍のまま dir 方向へ maxDistance だけ掃引する。凸半径を 0 にして指定の大きさちょうどにする。
		JPH::BoxShape boxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), 0.0f);
		JPH::RShapeCast shapeCast(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::RVec3(origin.x, origin.y, origin.z)), dir * maxDistance);

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display: what was asked now, what was found just before returning.
		/// [JP] クエリはすべてデバッグ表示のために記録する。問い合わせた内容はここで、結果は返す直前に入れる。
		QueryDesc query;
		query.kind_ = QueryKind::Boxcast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
		query.shape_ = ColliderKind::Box;
		query.dimensions_ = halfExtents;
		query.rotation_ = rotation;
#endif

		JPH::ShapeCastSettings settings;

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every hit, since the nearest one may be filtered out below.
		/// [JP] 最も近いヒットが下で除外されることもあるので、全てのヒットを集める。
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [EN] Nearest first, so the first hit that passes the filters is the answer.
		/// [JP] 近い順に並べ、フィルタを通った最初のヒットを答えにする。
		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			/// [EN] 2D canvas bodies share the Jolt world but are not part of 3D queries.
			/// [JP] 2D Canvas のボディは Jolt ワールドを共有するが、3D のクエリには含めない。
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [EN] The penetration axis points from the box into the hit body, so its negation is the surface normal.
			/// [JP] めり込み軸は箱から相手のボディへ向くので、その逆向きが面の法線になる。
			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			/// [EN] The contact point is taken on the hit body's surface.
			/// [JP] 接触点は、当たったボディの表面上の点を使う。
			outHit.position_ = Vector3(hit.mContactPointOn2.GetX(), hit.mContactPointOn2.GetY(), hit.mContactPointOn2.GetZ());
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [EN]
	* Returns entities overlapping a 3D query shape of the given kind and
	* size, placed at the given pose. Only Sphere, Box, Capsule and
	* Cylinder are 3D kinds; any other kind overlaps nothing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した種類と大きさの 3D クエリ形状を指定の姿勢に置き、重なる
	* エンティティを返す。3D の種類は Sphere・Box・Capsule・Cylinder
	* だけで、それ以外の種類は何とも重ならない。
	*/
	DynamicArray<EntityID> Physics::Overlap(OverlapShapeKind kind, const Vector3& dimensions, const Vector3& position, const Quaternion& rotation, Uint32 layerMask)const
	{
		DynamicArray<EntityID> result;

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display; the kind and size come straight from the arguments, in the same layout as ColliderDesc::dimensions_.
		/// [JP] クエリはすべてデバッグ表示のために記録する。種類と大きさは引数そのままで、並びは ColliderDesc::dimensions_ と同じ。
		QueryDesc query;
		query.kind_ = QueryKind::Overlap;
		query.origin_ = position;
		query.rotation_ = rotation;
		query.dimensions_ = dimensions;
#endif

		/// [EN] The query shape lives only for this call and is freed by its reference count on return; it is never registered in the shape pool, so the caller has nothing to release.
		/// [JP] クエリ形状はこの呼び出しの間だけ存在し、戻るときに参照カウントで解放される。形状プールには登録しないので、呼び出し側が解放するものは無い。
		JPH::ShapeRefC queryShape;
		switch (kind)
		{
		case OverlapShapeKind::Sphere:
		{
			queryShape = new JPH::SphereShape(dimensions.x);
#if SC_DEBUG
			query.shape_ = ColliderKind::Sphere;
#endif
			break;
		}
		case OverlapShapeKind::Box:
		{
			/// [EN] A zero convex radius keeps the box exactly the given size and lets a side be thinner than Jolt's default convex radius.
			/// [JP] 凸半径を 0 にすると、箱は指定の大きさちょうどになり、Jolt の既定の凸半径より薄い辺も持てる。
			queryShape = new JPH::BoxShape(JPH::Vec3(dimensions.x, dimensions.y, dimensions.z), 0.0f);
#if SC_DEBUG
			query.shape_ = ColliderKind::Box;
#endif
			break;
		}
		case OverlapShapeKind::Capsule:
		{
			queryShape = new JPH::CapsuleShape(dimensions.y, dimensions.x);
#if SC_DEBUG
			query.shape_ = ColliderKind::Capsule;
#endif
			break;
		}
		case OverlapShapeKind::Cylinder:
		{
			/// [EN] A zero convex radius, for the same reason as Box.
			/// [JP] 凸半径を 0 にする。理由は Box と同じ。
			queryShape = new JPH::CylinderShape(dimensions.y, dimensions.x, 0.0f);
#if SC_DEBUG
			query.shape_ = ColliderKind::Cylinder;
#endif
			break;
		}
		default:
		{
			/// [EN] Canvas kinds have no 3D shape, so they overlap nothing.
			/// [JP] Canvas の種類には 3D の形が無いので、何とも重ならない。
			return result;
		}
		}

		/// [EN] Places the query shape at the given pose.
		/// [JP] クエリ形状を指定の姿勢に置く。
		JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::RVec3(position.x, position.y, position.z));

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every overlapping body at unit scale; the order carries no meaning.
		/// [JP] 等倍で重なる全てのボディを集める。順番に意味は無い。
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CollideShape(queryShape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

		for (const JPH::CollideShapeResult& hit : collector.mHits)
		{
			/// [EN] 2D canvas bodies share the Jolt world but are not part of 3D queries.
			/// [JP] 2D Canvas のボディは Jolt ワールドを共有するが、3D のクエリには含めない。
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			result.push_back(BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
		}

#if SC_DEBUG
		query.hit_ = !result.empty();
		queryInstance_.Add(query);
#endif

		return result;
	}

	/**
	* [EN]
	* Finds the nearest planar body intersected by a 2D ray.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2Dレイと交差する最も近い平面ボディを検索する。
	*/
	Bool Physics::Raycast2D(const Vector2& origin, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		/// [EN] Canvas Y points down and physics Y points up, so Y is negated; 2D bodies live on the z = 0 plane.
		/// [JP] Canvas の Y は下向き、物理の Y は上向きなので Y を反転する。2D ボディは z = 0 の平面上にある。
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		/// [EN] The origin and reach are converted from pixels to meters.
		/// [JP] 始点と届く距離をピクセルからメートルへ変換する。
		JPH::RRayCast ray(JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f), dir * (maxDistance / pixelsPerMeter_));

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display, in canvas pixels with Y down: what was asked now, what was found just before returning.
		/// [JP] クエリはすべてデバッグ表示のために、Y 下向きの Canvas ピクセルで記録する。問い合わせた内容はここで、結果は返す直前に入れる。
		QueryDesc query;
		query.kind_ = QueryKind::Raycast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
#endif

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every hit, since the nearest one may be filtered out below.
		/// [JP] 最も近いヒットが下で除外されることもあるので、全てのヒットを集める。
		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [EN] Nearest first, so the first hit that passes the filters is the answer.
		/// [JP] 近い順に並べ、フィルタを通った最初のヒットを答えにする。
		collector.Sort();

		for (const JPH::RayCastResult& hit : collector.mHits)
		{
			/// [EN] Only 2D canvas bodies take part in 2D queries.
			/// [JP] 2D のクエリには、2D Canvas のボディだけを含める。
			if (!(physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID) & Layers::PLANAR))
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [EN] The hit point lies at the hit fraction along the ray.
			/// [JP] ヒット位置は、レイ上のヒット割合の位置にある。
			JPH::RVec3 hitPosition = ray.GetPointOnRay(hit.mFraction);

			/// [EN] The surface normal needs the body's shape, so the body is read-locked briefly.
			/// [JP] 面の法線はボディの形状から求めるので、短い間だけ読み込みロックをかける。
			JPH::Vec3 normal = JPH::Vec3::sZero();
			{
				JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
				if (lock.Succeeded())
				{
					normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPosition);
				}
			}

			/// [EN] Results go back to canvas pixels with Y down; the distance scales maxDistance, already in pixels.
			/// [JP] 結果は Y 下向きの Canvas ピクセルへ戻す。距離はピクセル単位の maxDistance に割合を掛ける。
			outHit.position_ = Vector2(static_cast<Float>(hitPosition.GetX()) * pixelsPerMeter_, -static_cast<Float>(hitPosition.GetY()) * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [EN]
	* Finds the nearest planar body intersected by a swept circle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 移動する円と交差する最も近い平面ボディを検索する。
	*/
	Bool Physics::Circlecast2D(const Vector2& origin, Float radius, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		/// [EN] Canvas Y points down and physics Y points up, so Y is negated; 2D bodies live on the z = 0 plane.
		/// [JP] Canvas の Y は下向き、物理の Y は上向きなので Y を反転する。2D ボディは z = 0 の平面上にある。
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		/// [EN] On the z = 0 plane a sphere cuts out the circle; radius, origin and reach go from pixels to meters.
		/// [JP] z = 0 の平面上では球の断面が円になる。半径・始点・届く距離はピクセルからメートルへ変換する。
		JPH::SphereShape sphereShape(radius / pixelsPerMeter_);
		JPH::RShapeCast shapeCast(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f)), dir * (maxDistance / pixelsPerMeter_));

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display, in canvas pixels with Y down: what was asked now, what was found just before returning.
		/// [JP] クエリはすべてデバッグ表示のために、Y 下向きの Canvas ピクセルで記録する。問い合わせた内容はここで、結果は返す直前に入れる。
		QueryDesc query;
		query.kind_ = QueryKind::Circlecast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
		query.radius_ = radius;
#endif

		JPH::ShapeCastSettings settings;
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every hit, since the nearest one may be filtered out below.
		/// [JP] 最も近いヒットが下で除外されることもあるので、全てのヒットを集める。
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [EN] Nearest first, so the first hit that passes the filters is the answer.
		/// [JP] 近い順に並べ、フィルタを通った最初のヒットを答えにする。
		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			/// [EN] Only 2D canvas bodies take part in 2D queries.
			/// [JP] 2D のクエリには、2D Canvas のボディだけを含める。
			if (!(physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR))
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [EN] The penetration axis points from the circle into the hit body, so its negation is the surface normal.
			/// [JP] めり込み軸は円から相手のボディへ向くので、その逆向きが面の法線になる。
			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			/// [EN] Results go back to canvas pixels with Y down; the distance scales maxDistance, already in pixels.
			/// [JP] 結果は Y 下向きの Canvas ピクセルへ戻す。距離はピクセル単位の maxDistance に割合を掛ける。
			outHit.position_ = Vector2(hit.mContactPointOn2.GetX() * pixelsPerMeter_, -hit.mContactPointOn2.GetY() * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	Bool Physics::Boxcast2D(const Vector2& origin, const Vector2& halfExtents, Float rotation, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::BoxShape boxShape(JPH::Vec3(halfExtents.x / pixelsPerMeter_, halfExtents.y / pixelsPerMeter_, 0.5f), 0.0f);
		JPH::RShapeCast shapeCast(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -ToRadians(rotation)), JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f)), dir * (maxDistance / pixelsPerMeter_));

#if SC_DEBUG
		QueryDesc query;
		query.kind_ = QueryKind::Boxcast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
		query.shape_ = ColliderKind::Rect;
		query.dimensions_ = Vector3(halfExtents.x, halfExtents.y, 0.0f);
		query.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, ToRadians(rotation));
#endif

		JPH::ShapeCastSettings settings;
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG
			queryInstance_.Add(query);
#endif
			return false;
		}

		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			if (!(physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR))
			{
				continue;
			}

			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			outHit.position_ = Vector2(hit.mContactPointOn2.GetX() * pixelsPerMeter_, -hit.mContactPointOn2.GetY() * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [EN]
	* Returns entities overlapping a canvas query shape of the given kind
	* and size, placed at the given canvas position (pixels, Y down) and
	* rotation (degrees). Only Circle and Rect are canvas kinds; any other
	* kind overlaps nothing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した種類と大きさの Canvas クエリ形状を、指定の Canvas 上の位置
	* （ピクセル、Y 下向き）と回転（度）に置き、重なるエンティティを返す。
	* Canvas の種類は Circle と Rect だけで、それ以外の種類は何とも
	* 重ならない。
	*/
	DynamicArray<EntityID> Physics::Overlap2D(OverlapShapeKind kind, const Vector2& dimensions, const Vector2& position, Float rotation, Uint32 layerMask)const
	{
		DynamicArray<EntityID> result;

#if SC_DEBUG
		/// [EN] Every query is recorded for the debug display in canvas pixels with Y down, the rotation turning about Z by the canvas angle; the size comes straight from the arguments.
		/// [JP] クエリはすべてデバッグ表示のために、Y 下向きの Canvas ピクセルで記録し、回転は Canvas の角度だけ Z 軸まわりに回す。大きさは引数そのまま。
		QueryDesc query;
		query.kind_ = QueryKind::Overlap2D;
		query.origin_ = Vector3(position.x, position.y, 0.0f);
		query.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, ToRadians(rotation));
		query.dimensions_ = Vector3(dimensions.x, dimensions.y, 0.0f);
#endif

		/// [EN] The query shape lives only for this call and is freed by its reference count on return; it is never registered in the shape pool, so the caller has nothing to release. Its size goes from pixels to meters.
		/// [JP] クエリ形状はこの呼び出しの間だけ存在し、戻るときに参照カウントで解放される。形状プールには登録しないので、呼び出し側が解放するものは無い。大きさはピクセルからメートルへ変換する。
		JPH::ShapeRefC queryShape;
		switch (kind)
		{
		case OverlapShapeKind::Circle:
		{
			/// [EN] A circle is a sphere centered on the canvas plane, so its cross-section there is the circle itself.
			/// [JP] 円は Canvas の平面上に中心を置いた球なので、その平面での断面がそのまま円になる。
			queryShape = new JPH::SphereShape(dimensions.x / pixelsPerMeter_);
#if SC_DEBUG
			query.shape_ = ColliderKind::Circle;
#endif
			break;
		}
		case OverlapShapeKind::Rect:
		{
			/// [EN] A rectangle is a box with the same half thickness as canvas bodies, and a zero convex radius so it is exactly the given size.
			/// [JP] 矩形は Canvas のボディと同じ厚みの半分を持つ箱で、凸半径を 0 にして指定の大きさちょうどにする。
			queryShape = new JPH::BoxShape(JPH::Vec3(dimensions.x / pixelsPerMeter_, dimensions.y / pixelsPerMeter_, 0.5f), 0.0f);
#if SC_DEBUG
			query.shape_ = ColliderKind::Rect;
#endif
			break;
		}
		default:
		{
			/// [EN] 3D kinds have no canvas shape, so they overlap nothing.
			/// [JP] 3D の種類には Canvas の形が無いので、何とも重ならない。
			return result;
		}
		}

		/// [EN] Rotation is in degrees about Z and negated with the flipped Y; position goes from pixels to meters.
		/// [JP] 回転は Z 軸まわりの度で、Y の反転に合わせて符号を反転する。位置はピクセルからメートルへ変換する。
		JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -ToRadians(rotation)), JPH::RVec3(position.x / pixelsPerMeter_, -position.y / pixelsPerMeter_, 0.0f));

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [EN] Collects every overlapping body at unit scale; the order carries no meaning.
		/// [JP] 等倍で重なる全てのボディを集める。順番に意味は無い。
		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CollideShape(queryShape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

		for (const JPH::CollideShapeResult& hit : collector.mHits)
		{
			/// [EN] Only 2D canvas bodies take part in 2D queries.
			/// [JP] 2D のクエリには、2D Canvas のボディだけを含める。
			if (!(physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR))
			{
				continue;
			}

			/// [EN] Skips bodies whose actor's layer bit is not in layerMask; all bits set means no filter.
			/// [JP] Actor のレイヤーのビットが layerMask に無いボディは飛ばす。全ビットが立っていればフィルタ無し。
			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			result.push_back(BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
		}

#if SC_DEBUG
		query.hit_ = !result.empty();
		queryInstance_.Add(query);
#endif

		return result;
	}

	/**
	* [EN]
	* Creates and registers a two-body constraint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2ボディ拘束を生成して登録する。
	*/
	ScConstraintHandle Physics::CreateConstraint(ScBodyID scBodyA, ScBodyID scBodyB, const JPH::TwoBodyConstraintSettings& settings)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		/// [EN] bodyA, the joint's owner, is required; bodyB is optional.
		/// [JP] ジョイントの持ち主である bodyA は必須、bodyB は省略できる。
		if (bodyA.IsInvalid())
		{
			return ScConstraintHandle::null();
		}

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();
		const JPH::BodyLockInterface& lockInterface = physicsSystem.GetBodyLockInterface();

		JPH::Constraint* constraint = nullptr;

		/// [EN] Without bodyB the joint attaches bodyA to the world, which takes the body1 slot.
		/// [JP] bodyB が無ければ、bodyA をワールドへ繋ぐ。ワールドが body1 の位置に入る。
		if (bodyB.IsInvalid())
		{
			JPH::BodyLockWrite lock(lockInterface, bodyA);
			if (!lock.Succeeded())
			{
				return ScConstraintHandle::null();
			}
			constraint = settings.Create(JPH::Body::sFixedToWorld, lock.GetBody());
		}
		else
		{
			/// [EN] bodyB is body1 and bodyA is body2, the same order in both cases.
			/// [JP] bodyB を body1、bodyA を body2 とし、どちらの場合も順番を揃える。
			JPH::BodyID bodyIDs[2] = { bodyB, bodyA };
			JPH::BodyLockMultiWrite lock(lockInterface, bodyIDs, 2);
			JPH::Body* body1 = lock.GetBody(0);
			JPH::Body* body2 = lock.GetBody(1);
			if (body1 == nullptr || body2 == nullptr)
			{
				return ScConstraintHandle::null();
			}
			constraint = settings.Create(*body1, *body2);
		}

		if (constraint == nullptr)
		{
			return ScConstraintHandle::null();
		}

		/// [EN] The pool adds the constraint to the simulation and hands back a generation-checked handle.
		/// [JP] プールが拘束をシミュレーションへ加え、世代付きのハンドルを返す。
		return joltManager_.ConstraintPool().Add(physicsSystem, constraint);
	}

}
