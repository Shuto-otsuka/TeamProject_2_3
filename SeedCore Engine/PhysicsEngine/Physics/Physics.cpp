#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Core/Array.h>
#include <External/JoltPhysics/Jolt/Math/Float3.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Body/Body.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyCreationSettings.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyFilter.h>
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
	* [JP]
	* 使う世界と、スクリプトから呼ぶ窓口を受け取る。
	*
	* @brief 準備する
	* @param world         使う世界
	* @param queryInstance スクリプトから呼ぶ窓口
	*
	* [EN]
	* Takes the world to use and the door scripts call through.
	*
	* @brief Gets ready
	* @param world         The world to use
	* @param queryInstance The door scripts call through
	*/
	Physics::Physics(World& world, QueryInstance& queryInstance) :joltManager_(Gateway::GetJoltManager()), world_(world), queryInstance_(queryInstance)
	{
		/// No Code
	}

	/**
	* [JP]
	* 箱の形を作る。
	*
	* @brief  箱の形を作る
	* @param  size   大きさ
	* @param  center 中心のずれ
	* @return 形の番号
	*
	* [EN]
	* Makes a box shape.
	*
	* @brief  Makes a box shape
	* @param  size   The size
	* @param  center How far the center is moved
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateBoxShape(const Vector3& size, const Vector3& center)
	{
		return joltManager_.ShapePool().CreateBoxShape(size, center);
	}

	/**
	* [JP]
	* 球の形を作る。
	*
	* @brief  球の形を作る
	* @param  radius 半径
	* @return 形の番号
	*
	* [EN]
	* Makes a sphere shape.
	*
	* @brief  Makes a sphere shape
	* @param  radius The radius
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateSphereShape(Float radius)
	{
		return joltManager_.ShapePool().CreateSphereShape(radius);
	}

	/**
	* [JP]
	* カプセルの形を作る。
	*
	* @brief  カプセルの形を作る
	* @param  height 円柱部分の高さ
	* @param  radius 半径
	* @return 形の番号
	*
	* [EN]
	* Makes a capsule shape.
	*
	* @brief  Makes a capsule shape
	* @param  height The height of the middle part
	* @param  radius The radius
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateCapsuleShape(Float height, Float radius)
	{
		return joltManager_.ShapePool().CreateCapsuleShape(height, radius);
	}

	/**
	* [JP]
	* 円柱の形を作る。
	*
	* @brief  円柱の形を作る
	* @param  height 高さ
	* @param  radius 半径
	* @return 形の番号
	*
	* [EN]
	* Makes a cylinder shape.
	*
	* @brief  Makes a cylinder shape
	* @param  height The height
	* @param  radius The radius
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateCylinderShape(Float height, Float radius)
	{
		return joltManager_.ShapePool().CreateCylinderShape(height, radius);
	}

	/**
	* [JP]
	* 2D の四角の形を作る。大きさはピクセルで渡す。
	*
	* @brief  四角の形を作る
	* @param  size   大きさ（ピクセル）
	* @param  center 中心のずれ（ピクセル）
	* @return 形の番号
	*
	* [EN]
	* Makes a 2D rectangle shape; the size is given in pixels.
	*
	* @brief  Makes a rectangle shape
	* @param  size   The size in pixels
	* @param  center How far the center is moved, in pixels
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateRectShape(const Vector2& size, const Vector2& center)
	{
		/// [JP] 2D はピクセルをメートルに直し、Y を逆向きにする
		/// [EN] For 2D, turn pixels into meters and flip Y
		return joltManager_.ShapePool().CreateRectShape(Vector2(size.x / pixelsPerMeter_, size.y / pixelsPerMeter_), Vector2(center.x / pixelsPerMeter_, -center.y / pixelsPerMeter_));
	}

	/**
	* [JP]
	* 2D の円の形を作る。大きさはピクセルで渡す。
	*
	* @brief  円の形を作る
	* @param  radius 半径（ピクセル）
	* @param  center 中心のずれ（ピクセル）
	* @return 形の番号
	*
	* [EN]
	* Makes a 2D circle shape; the size is given in pixels.
	*
	* @brief  Makes a circle shape
	* @param  radius The radius in pixels
	* @param  center How far the center is moved, in pixels
	* @return The shape number
	*/
	ScShapeHandle Physics::CreateCircleShape(Float radius, const Vector2& center)
	{
		/// [JP] 2D はピクセルをメートルに直し、Y を逆向きにする
		/// [EN] For 2D, turn pixels into meters and flip Y
		return joltManager_.ShapePool().CreateCircleShape(radius / pixelsPerMeter_, Vector2(center.x / pixelsPerMeter_, -center.y / pixelsPerMeter_));
	}

	/**
	* [JP]
	* 三角形の集まり（メッシュ）から形を作る。
	*
	* @brief  メッシュの形を作る
	* @param  assetID   メッシュのアセット番号
	* @param  positions 頂点の位置
	* @param  indices   三角形の頂点の並び
	* @param  scale     大きさ
	* @return 形の番号。作れなければ空
	* @note   動く体には使えない
	*
	* [EN]
	* Makes a shape from a set of triangles (a mesh).
	*
	* @brief  Makes a mesh shape
	* @param  assetID   The mesh asset number
	* @param  positions The vertex positions
	* @param  indices   The vertex order of the triangles
	* @param  scale     The scale
	* @return The shape number, or empty if it could not be made
	* @note   Not usable on moving bodies
	*/
	ScShapeHandle Physics::CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale)
	{
		return joltManager_.ShapePool().CreateMeshShape(assetID, positions, indices, scale);
	}

	/**
	* [JP]
	* 点の集まりを包む、へこみのない形（凸包）を作る。
	*
	* @brief  凸包の形を作る
	* @param  assetID   メッシュのアセット番号
	* @param  positions 包む点の位置
	* @param  scale     大きさ
	* @return 形の番号。作れなければ空
	*
	* [EN]
	* Makes a dent-free shape (a convex hull) that wraps a set of points.
	*
	* @brief  Makes a convex shape
	* @param  assetID   The mesh asset number
	* @param  positions The points to wrap
	* @param  scale     The scale
	* @return The shape number, or empty if it could not be made
	*/
	ScShapeHandle Physics::CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale)
	{
		return joltManager_.ShapePool().CreateConvexShape(assetID, positions, scale);
	}

	/**
	* [JP]
	* 形を使い終わったことを伝える。誰も使わなくなったら消える。
	*
	* @brief 形を手放す
	* @param handle 形の番号
	*
	* [EN]
	* Says you are done with a shape; it is removed once nobody uses it.
	*
	* @brief Releases a shape
	* @param handle The shape number
	*/
	void Physics::ReleaseShape(ScShapeHandle handle)
	{
		joltManager_.ShapePool().Release(handle);
	}

	/**
	* [JP]
	* 坂や段差を歩けるキャラクターを作る。
	*
	* @brief  キャラクターを作る
	* @param  desc キャラクターの設定
	* @return キャラクターの目印
	*
	* [EN]
	* Makes a character that can walk up slopes and steps.
	*
	* @brief  Makes a character
	* @param  desc The character settings
	* @return The character handle
	*/
	ScCharacterHandle Physics::CreateCharacter(const CharacterDesc& desc)
	{
		/// [JP] 足元が原点になるように、カプセルを上にずらす
		/// [EN] Raise the capsule so its feet sit at the origin
		JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(desc.height_ * 0.5f, desc.radius_);

		JPH::RefConst<JPH::Shape> shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0.0f, desc.height_ * 0.5f + desc.radius_, 0.0f), JPH::Quat::sIdentity(), capsule);

		/// [JP] 登れる坂・重さ・押す強さを決める
		/// [EN] Set the slope limit, mass and pushing strength
		JPH::CharacterVirtualSettings settings;
		settings.mShape = shape;
		settings.mMaxSlopeAngle = desc.maxSlopeAngle_;
		settings.mMass = desc.mass_;
		settings.mMaxStrength = desc.maxStrength_;

		/// [JP] ほかの体から見える中身の体も同じ形にする
		/// [EN] Give the inner body that others can see the same shape
		settings.mInnerBodyShape = settings.mShape;
		settings.mInnerBodyLayer = Layers::Pack(Layers::KINEMATIC, Layers::UnpackUserLayer(desc.layer_));

		settings.mEnhancedInternalEdgeRemoval = true;

		/// [JP] キャラクターを作って、持っておく
		/// [EN] Create the character and hold on to it
		JPH::CharacterVirtual* character = new JPH::CharacterVirtual(&settings, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), std::bit_cast<JPH::uint64>(desc.userData_), &joltManager_.PhysicsSystem());
		character->AddRef();
		return character;
	}

	/**
	* [JP]
	* キャラクターを1回分動かす。
	*
	* @brief キャラクターを動かす
	* @param character     動かすキャラクター
	* @param elapsedTime   進める時間
	* @param maxSlopeAngle 登れる坂の一番急な角度
	* @param stepHeight    登れる段差の高さ
	*
	* [EN]
	* Moves the character one step.
	*
	* @brief Moves the character
	* @param character     The character to move
	* @param elapsedTime   How much time to move
	* @param maxSlopeAngle The steepest slope it can climb
	* @param stepHeight    How high a step it can climb
	*/
	void Physics::UpdateCharacter(ScCharacterHandle handle, Float elapsedTime, Float maxSlopeAngle, Float stepHeight)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return;
		}

		/// [JP] 登れる坂と段差の高さを決めて、1回分動かす
		/// [EN] Set the slope and step limits, then move one step
		character->SetMaxSlopeAngle(maxSlopeAngle);

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
		updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, stepHeight, 0.0f);

		character->ExtendedUpdate(elapsedTime, physicsSystem.GetGravity(), updateSettings, physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator());
	}

	/**
	* [JP]
	* キャラクターが乗っている地面の情報を新しくする。
	*
	* @brief 地面の情報を新しくする
	* @param character 調べるキャラクター
	*
	* [EN]
	* Updates what the character knows about the ground under it.
	*
	* @brief Refreshes the ground info
	* @param character The character to refresh
	*/
	void Physics::RefreshCharacter(ScCharacterHandle handle)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return;
		}

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		character->RefreshContacts(physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator());
	}

	/**
	* [JP]
	* キャラクターを消す。
	*
	* @brief キャラクターを消す
	* @param character 消すキャラクター。消したあとは空になる
	*
	* [EN]
	* Removes a character.
	*
	* @brief Removes a character
	* @param character The character to remove; it becomes empty afterwards
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
	* [JP]
	* キャラクターの体の高さと半径を変える。周りにぶつかるなら変えない。
	*
	* @brief  体の大きさを変える
	* @param  character 変えるキャラクター
	* @param  height    新しい高さ
	* @param  radius    新しい半径
	* @return 変えられたら true
	*
	* [EN]
	* Changes the character's body height and radius, unless it would hit something.
	*
	* @brief  Resizes the body
	* @param  character The character to change
	* @param  height    The new height
	* @param  radius    The new radius
	* @return True if it changed
	*/
	Bool Physics::CharacterHeight(ScCharacterHandle handle, Float height, Float radius)
	{
		JPH::CharacterVirtual* character = static_cast<JPH::CharacterVirtual*>(handle);
		if (!character)
		{
			return false;
		}

		/// [JP] 新しい大きさのカプセルを作る
		/// [EN] Build a capsule of the new size
		JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(height * 0.5f, radius);
		JPH::RefConst<JPH::Shape> shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0.0f, height * 0.5f + radius, 0.0f), JPH::Quat::sIdentity(), capsule);

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [JP] 周りにぶつかって変えられなければ、そのままにする
		/// [EN] If it would hit something, keep the old shape
		if (!character->SetShape(shape, 0.05f, physicsSystem.GetDefaultBroadPhaseLayerFilter(Layers::DYNAMIC), physicsSystem.GetDefaultLayerFilter(Layers::DYNAMIC), { }, { }, joltManager_.PhysicsAllocator()))
		{
			return false;
		}

		character->SetInnerBodyShape(shape);
		return true;
	}

	/**
	* [JP]
	* 重力の向きと強さを返す。
	*
	* @brief  重力を返す
	* @return 重力
	*
	* [EN]
	* Returns the direction and strength of gravity.
	*
	* @brief  Returns gravity
	* @return Gravity
	*/
	Vector3 Physics::Gravity()const
	{
		JPH::Vec3 gravity = joltManager_.PhysicsSystem().GetGravity();
		return Vector3(gravity.GetX(), gravity.GetY(), gravity.GetZ());
	}

	/**
	* [JP]
	* 体（Rigidbody）を作る。
	*
	* @brief  体を作る
	* @param  desc 体の設定
	* @return 体の番号。作れなければ無効な番号
	*
	* [EN]
	* Makes a body (Rigidbody).
	*
	* @brief  Makes a body
	* @param  desc The body settings
	* @return The body number, or the invalid number if it could not be made
	*/
	ScBodyID Physics::CreateRigidbody(const RigidbodyDesc& desc)
	{
		/// [JP] 形がなければ体は作れない
		/// [EN] No shape means no body
		JPH::ShapeRefC shape = joltManager_.ShapePool().Get(desc.shape_);
		if (!shape)
		{
			return ScInvalidBodyID;
		}

		/// [JP] 位置・向き・動き方・層で体の設定を作る
		/// [EN] Build the body settings from position, facing, motion and layer
		JPH::BodyCreationSettings settings(shape, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), static_cast<JPH::EMotionType>(desc.motionType_), desc.layer_);

		settings.mAllowedDOFs = static_cast<JPH::EAllowedDOFs>(desc.allowedDOFs_);
		settings.mLinearDamping = desc.linearDamping_;
		settings.mAngularDamping = desc.angularDamping_;
		settings.mFriction = desc.friction_;
		settings.mRestitution = desc.restitution_;
		settings.mGravityFactor = desc.gravityFactor_;
		settings.mUserData = std::bit_cast<JPH::uint64>(desc.userData_);
		settings.mIsSensor = desc.isSensor_;

		/// [JP] 動き方を変えられるか、プログラムで動く重なり用の体が止まった体ともぶつかるかを決める
		/// [EN] Decide whether the motion can change and whether a code-moved overlap body meets still bodies
		settings.mAllowDynamicOrKinematic = desc.allowMotionTypeChange_;

		settings.mCollideKinematicVsNonDynamic = desc.motionType_ == ScMotionType::Kinematic && desc.isSensor_;

		settings.mAllowSleeping = desc.motionType_ != ScMotionType::Kinematic;

		/// [JP] 動く体は重さと、すり抜けを防ぐかを決める
		/// [EN] For moving bodies, set the mass and whether to stop tunneling
		if (desc.motionType_ != ScMotionType::Static || desc.allowMotionTypeChange_)
		{
			settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			settings.mMassPropertiesOverride.mMass = desc.mass_;

			settings.mMotionQuality = desc.continuousCollision_ ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
		}

		/// [JP] 体を作って世界に入れる
		/// [EN] Create the body and add it to the world
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::Body* body = bodyInterface.CreateBody(settings);
		if (!body)
		{
			return ScInvalidBodyID;
		}

		bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
		return body->GetID().GetIndexAndSequenceNumber();
	}

	/**
	* [JP]
	* やわらかい体（Softbody）を作る。
	*
	* @brief  やわらかい体を作る
	* @param  desc やわらかい体の設定
	* @return 体の番号。作れなければ無効な番号
	*
	* [EN]
	* Makes a soft body.
	*
	* @brief  Makes a soft body
	* @param  desc The soft body settings
	* @return The body number, or the invalid number if it could not be made
	*/
	ScBodyID Physics::CreateSoftbody(const SoftbodyDesc& desc)
	{
		/// [JP] 頂点を写す
		/// [EN] Copy the vertices
		JPH::Ref<JPH::SoftBodySharedSettings> sharedSettings = new JPH::SoftBodySharedSettings();

		sharedSettings->mVertices.reserve(desc.positions_.size());
		for (const Vector3& position : desc.positions_)
		{
			sharedSettings->mVertices.push_back(JPH::SoftBodySharedSettings::Vertex(JPH::Float3(position.x, position.y, position.z)));
		}

		/// [JP] 三角形を写す。おかしな三角形は飛ばす
		/// [EN] Copy the triangles, skipping broken ones
		Uint32 vertexCount = static_cast<Uint32>(desc.positions_.size());

		sharedSettings->mFaces.reserve(desc.indices_.size() / 3);
		for (Size faceIndex = 0; faceIndex + 2 < desc.indices_.size(); faceIndex += 3)
		{
			Uint32 vertex0 = desc.indices_[faceIndex];
			Uint32 vertex1 = desc.indices_[faceIndex + 1];
			Uint32 vertex2 = desc.indices_[faceIndex + 2];

			if (vertex0 >= vertexCount || vertex1 >= vertexCount || vertex2 >= vertexCount)
			{
				SC_LOG_WARNING("Softbody: 頂点数({})を超える頂点インデックス({}, {}, {})を持つ面をスキップしました", vertexCount, vertex0, vertex1, vertex2);
				continue;
			}

			JPH::SoftBodySharedSettings::Face face(vertex0, vertex1, vertex2);
			if (face.IsDegenerate())
			{
				continue;
			}

			sharedSettings->AddFace(face);
		}

		if (sharedSettings->mFaces.empty())
		{
			return ScInvalidBodyID;
		}

		/// [JP] 辺のつながりから、伸びや曲がりの決まりを作る
		/// [EN] Build the stretch and bend rules from the edges
		JPH::SoftBodySharedSettings::VertexAttributes vertexAttributes(desc.edgeCompliance_, desc.shearCompliance_, desc.bendCompliance_);
		sharedSettings->CreateConstraints(&vertexAttributes, 1);

		sharedSettings->Optimize();

		/// [JP] 動きの設定をして、世界に入れる
		/// [EN] Set up the motion and add it to the world
		JPH::SoftBodyCreationSettings settings(sharedSettings, JPH::RVec3(desc.position_.x, desc.position_.y, desc.position_.z), JPH::Quat(desc.rotation_.x, desc.rotation_.y, desc.rotation_.z, desc.rotation_.w), desc.layer_);
		settings.mNumIterations = desc.numIterations_;
		settings.mLinearDamping = desc.linearDamping_;
		settings.mPressure = desc.pressure_;
		settings.mFriction = desc.friction_;
		settings.mRestitution = desc.restitution_;
		settings.mGravityFactor = desc.gravityFactor_;
		settings.mUserData = std::bit_cast<JPH::uint64>(desc.userData_);

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		return bodyInterface.CreateAndAddSoftBody(settings, JPH::EActivation::Activate).GetIndexAndSequenceNumber();
	}

	/**
	* [JP]
	* 体を世界から一時的に外す。
	*
	* @brief 体を止める
	* @param bodyID 体の番号
	*
	* [EN]
	* Takes a body out of the world for now.
	*
	* @brief Suspends a body
	* @param bodyID The body number
	*/
	void Physics::SuspendBody(ScBodyID bodyID)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		/// [JP] 世界に入っていれば外す
		/// [EN] Remove it from the world if it is in
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.IsAdded(physicsBodyID))
		{
			bodyInterface.RemoveBody(physicsBodyID);
		}
	}

	/**
	* [JP]
	* 外した体を世界に戻す。
	*
	* @brief 体を戻す
	* @param bodyID 体の番号
	*
	* [EN]
	* Puts a suspended body back into the world.
	*
	* @brief Resumes a body
	* @param bodyID The body number
	*/
	void Physics::ResumeBody(ScBodyID bodyID)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		/// [JP] 世界に入っていなければ戻す
		/// [EN] Put it back if it is not in the world
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (!bodyInterface.IsAdded(physicsBodyID))
		{
			bodyInterface.AddBody(physicsBodyID, JPH::EActivation::Activate);
		}
	}

	/**
	* [JP]
	* 体を消す。
	*
	* @brief 体を消す
	* @param bodyID 体の番号
	*
	* [EN]
	* Removes a body.
	*
	* @brief Removes a body
	* @param bodyID The body number
	*/
	void Physics::DestroyBody(ScBodyID bodyID)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		/// [JP] 世界から外してから消す
		/// [EN] Take it out of the world, then remove it
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.IsAdded(physicsBodyID))
		{
			bodyInterface.RemoveBody(physicsBodyID);
		}
		bodyInterface.DestroyBody(physicsBodyID);
	}

	/**
	* [JP]
	* 体の形を変える。
	*
	* @brief 体の形を変える
	* @param bodyID 体の番号
	* @param shape  新しい形
	*
	* [EN]
	* Changes a body's shape.
	*
	* @brief Changes the shape
	* @param bodyID The body number
	* @param shape  The new shape
	*/
	void Physics::BodyShape(ScBodyID bodyID, ScShapeHandle shape)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::ShapeRefC newShape = joltManager_.ShapePool().Get(shape);
		if (!newShape)
		{
			return;
		}

		/// [JP] 形を入れ替える
		/// [EN] Swap in the new shape
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		bodyInterface.SetShape(physicsBodyID, newShape, false, JPH::EActivation::Activate);

		/// [JP] 重さを保ったまま、新しい形に合わせて回りにくさを計算し直す
		/// [EN] Keep the mass and recompute how hard it is to spin for the new shape
		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), physicsBodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		JPH::Body& body = lock.GetBody();
		JPH::MotionProperties* motionProperties = body.GetMotionPropertiesUnchecked();
		if (!motionProperties)
		{
			return;
		}

		Float inverseMass = motionProperties->GetInverseMassUnchecked();
		if (inverseMass <= 0.0f)
		{
			return;
		}

		JPH::MassProperties massProperties = newShape->GetMassProperties();
		massProperties.ScaleToMass(1.0f / inverseMass);
		motionProperties->SetMassProperties(motionProperties->GetAllowedDOFs(), massProperties);
	}

	/**
	* [JP]
	* 体の位置と向きを返す。
	*
	* @brief  位置と向きを返す
	* @param  bodyID 体の番号
	* @return 位置と向きをまとめた行列
	*
	* [EN]
	* Returns a body's position and facing.
	*
	* @brief  Returns the transform
	* @param  bodyID The body number
	* @return A matrix holding the position and facing
	*/
	Matrix Physics::BodyTransform(ScBodyID bodyID)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return Matrix::Identity;
		}

		/// [JP] 位置と向きを取り出して、1つの行列にまとめる
		/// [EN] Read the position and facing and combine them into one matrix
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 position = bodyInterface.GetPosition(physicsBodyID);
		JPH::Quat rotation = bodyInterface.GetRotation(physicsBodyID);

		return Matrix::CreateFromQuaternion(Quaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW())) * Matrix::CreateTranslation(Vector3(position.GetX(), position.GetY(), position.GetZ()));
	}

	/**
	* [JP]
	* 体の位置と向きを変える。
	*
	* @brief 位置と向きを変える
	* @param bodyID   体の番号
	* @param position 新しい位置
	* @param rotation 新しい向き
	*
	* [EN]
	* Changes a body's position and facing.
	*
	* @brief Sets the transform
	* @param bodyID   The body number
	* @param position The new position
	* @param rotation The new facing
	*/
	void Physics::BodyTransform(ScBodyID bodyID, const Vector3& position, const Quaternion& rotation)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		bodyInterface.SetPositionAndRotationWhenChanged(physicsBodyID, JPH::RVec3(position.x, position.y, position.z), JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w).Normalized(), JPH::EActivation::DontActivate);
	}

	/**
	* [JP]
	* 体の動く速さを返す。
	*
	* @brief  速さを返す
	* @param  bodyID 体の番号
	* @return 速さ
	*
	* [EN]
	* Returns how fast a body moves.
	*
	* @brief  Returns the velocity
	* @param  bodyID The body number
	* @return The velocity
	*/
	Vector3 Physics::BodyLinearVelocity(ScBodyID bodyID)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		JPH::Vec3 linearVelocity = joltManager_.BodyInterface().GetLinearVelocity(physicsBodyID);
		return Vector3(linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ());
	}

	/**
	* [JP]
	* 体の動く速さを変える。
	*
	* @brief 速さを変える
	* @param bodyID         体の番号
	* @param linearVelocity 新しい速さ
	*
	* [EN]
	* Changes how fast a body moves.
	*
	* @brief Sets the velocity
	* @param bodyID         The body number
	* @param linearVelocity The new velocity
	*/
	void Physics::BodyLinearVelocity(ScBodyID bodyID, const Vector3& linearVelocity)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		joltManager_.BodyInterface().SetLinearVelocity(physicsBodyID, JPH::Vec3(linearVelocity.x, linearVelocity.y, linearVelocity.z));
	}

	/**
	* [JP]
	* 体の動き方を変える。
	*
	* @brief 動き方を変える
	* @param bodyID     体の番号
	* @param motionType 新しい動き方
	*
	* [EN]
	* Changes how a body moves.
	*
	* @brief Changes the motion type
	* @param bodyID     The body number
	* @param motionType The new way of moving
	*/
	void Physics::BodyMotionType(ScBodyID bodyID, ScMotionType motionType)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		const JPH::EMotionType joltMotionType = static_cast<JPH::EMotionType>(motionType);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();

		/// [JP] 重さで動く体にしたいのに、動けない体として作られていたら変えられない
		/// [EN] A body made as fully still cannot be switched to a moving one
		{
			JPH::BodyLockRead lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), physicsBodyID);
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

		/// [JP] 動き方を変える。プログラムで動かす体は止めておく
		/// [EN] Change the motion type; a code-moved body is brought to rest
		bodyInterface.SetMotionType(physicsBodyID, joltMotionType, JPH::EActivation::Activate);

		if (joltMotionType == JPH::EMotionType::Kinematic)
		{
			bodyInterface.SetLinearAndAngularVelocity(physicsBodyID, JPH::Vec3::sZero(), JPH::Vec3::sZero());
		}

		/// [JP] 層の番号の動き方の部分も書き換える
		/// [EN] Rewrite the motion part of the layer number too
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

		JPH::ObjectLayer layer = bodyInterface.GetObjectLayer(physicsBodyID);
		layer = static_cast<JPH::ObjectLayer>((layer & ~((1u << Layers::MOTION_TYPE_BITS) - 1u)) | motionLayer);
		bodyInterface.SetObjectLayer(physicsBodyID, layer);

		/// [JP] 重なり用の扱いと、眠れるかを合わせる
		/// [EN] Update the overlap handling and whether it may sleep
		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), physicsBodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		JPH::Body& body = lock.GetBody();
		body.SetCollideKinematicVsNonDynamic(joltMotionType == JPH::EMotionType::Kinematic && body.IsSensor());

		if (body.GetMotionPropertiesUnchecked())
		{
			body.SetAllowSleeping(joltMotionType != JPH::EMotionType::Kinematic);
		}
	}

	/**
	* [JP]
	* 体の回る速さを返す。
	*
	* @brief  回る速さを返す
	* @param  bodyID 体の番号
	* @return 回る速さ
	*
	* [EN]
	* Returns how fast a body spins.
	*
	* @brief  Returns the spin
	* @param  bodyID The body number
	* @return The spin
	*/
	Vector3 Physics::BodyAngularVelocity(ScBodyID bodyID)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return { 0.0f, 0.0f, 0.0f };
		}

		JPH::Vec3 angularVelocity = joltManager_.BodyInterface().GetAngularVelocity(physicsBodyID);
		return Vector3(angularVelocity.GetX(), angularVelocity.GetY(), angularVelocity.GetZ());
	}

	/**
	* [JP]
	* 体の回る速さを変える。
	*
	* @brief 回る速さを変える
	* @param bodyID          体の番号
	* @param angularVelocity 新しい回る速さ
	*
	* [EN]
	* Changes how fast a body spins.
	*
	* @brief Sets the spin
	* @param bodyID          The body number
	* @param angularVelocity The new spin
	*/
	void Physics::BodyAngularVelocity(ScBodyID bodyID, const Vector3& angularVelocity)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		joltManager_.BodyInterface().SetAngularVelocity(physicsBodyID, JPH::Vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
	}

	/**
	* [JP]
	* 体の上の点が、この時間でどれだけ動くかを求める。足場に乗ったものを一緒に動かすのに使う。
	*
	* @brief 点の動きを求める
	* @param bodyID          体の番号
	* @param point           調べる点
	* @param elapsedTime     時間
	* @param outDisplacement 動いた量が入る
	*
	* [EN]
	* Works out how far a point on a body moves in this time; used to carry things standing on it.
	*
	* @brief Works out a point's movement
	* @param bodyID          The body number
	* @param point           The point to check
	* @param elapsedTime     The time
	* @param outDisplacement Gets how far it moved
	*/
	void Physics::BodyPointDisplacement(ScBodyID bodyID, const Vector3& point, Float elapsedTime, Vector3& outDisplacement)const
	{
		outDisplacement = Vector3::Zero;

		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		/// [JP] 体の中心・速さ・回る速さを取る
		/// [EN] Read the body's center, velocity and spin
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 centerOfMass = bodyInterface.GetCenterOfMassPosition(physicsBodyID);
		JPH::Vec3 linearVelocity = bodyInterface.GetLinearVelocity(physicsBodyID);
		JPH::Vec3 angularVelocity = bodyInterface.GetAngularVelocity(physicsBodyID);

		/// [JP] 点が中心のまわりをこの時間でどれだけ回るかを足す
		/// [EN] Add how far the point swings around the center in this time
		JPH::Vec3 offset = JPH::Vec3(JPH::RVec3(point.x, point.y, point.z) - centerOfMass);
		JPH::Vec3 rotatedOffset = offset;

		Float angle = angularVelocity.Length() * elapsedTime;
		if (angle > 0.0f)
		{
			rotatedOffset = JPH::Quat::sRotation(angularVelocity.Normalized(), angle) * offset;
		}

		JPH::Vec3 displacement = linearVelocity * elapsedTime + rotatedOffset - offset;
		outDisplacement = Vector3(displacement.GetX(), displacement.GetY(), displacement.GetZ());
	}

	/**
	* [JP]
	* 体の真下にある足場を探す。
	*
	* @brief 足場を探す
	* @param bodyID        体の番号
	* @param maxSlopeAngle 足場とみなす坂の一番急な角度
	* @param maxDistance   下を探す距離
	* @param outFootholdID 見つけた足場の番号が入る
	*
	* [EN]
	* Looks for a foothold right under a body.
	*
	* @brief Finds a foothold
	* @param bodyID        The body number
	* @param maxSlopeAngle The steepest slope that counts as ground
	* @param maxDistance   How far down to look
	* @param outFootholdID Gets the foothold's body number
	*/
	void Physics::BodyFoothold(ScBodyID bodyID, Float maxSlopeAngle, Float maxDistance, ScBodyID& outFootholdID)const
	{
		outFootholdID = ScInvalidBodyID;

		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		/// [JP] 体の形と位置を、ロックしている間に写しておく
		/// [EN] Copy the body's shape and position while it is locked
		JPH::RefConst<JPH::Shape> shape;
		JPH::RMat44 transform;
		JPH::ObjectLayer layer;
		{
			JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), physicsBodyID);
			if (!lock.Succeeded())
			{
				return;
			}

			const JPH::Body& body = lock.GetBody();
			shape = body.GetShape();
			transform = body.GetCenterOfMassTransform();
			layer = body.GetObjectLayer();
		}

		/// [JP] 体の形を下に動かして、当たるものを全部集める
		/// [EN] Sweep the body's shape downward and collect every hit
		JPH::RShapeCast shapeCast(shape, JPH::Vec3::sReplicate(1.0f), transform, JPH::Vec3(0.0f, -maxDistance, 0.0f));
		JPH::ShapeCastSettings settings;
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		JPH::IgnoreSingleBodyFilter bodyFilter(physicsBodyID);
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, transform.GetTranslation(), collector, physicsSystem.GetDefaultBroadPhaseLayerFilter(layer), physicsSystem.GetDefaultLayerFilter(layer), bodyFilter);

		if (!collector.HadHit())
		{
			return;
		}

		/// [JP] 近い順に、重なり用でなく急すぎない面を足場にする
		/// [EN] In nearest order, take the first surface that is not overlap-only and not too steep
		collector.Sort();

		Float minNormalY = Cos(maxSlopeAngle);
		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::SENSOR)
			{
				continue;
			}

			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();
			if (normal.GetY() < minNormalY)
			{
				continue;
			}

			outFootholdID = hit.mBodyID2.GetIndexAndSequenceNumber();
			return;
		}
	}

	/**
	* [JP]
	* 重なりを知らせるだけの体にするかを切り替える。
	*
	* @brief 重なり専用を切り替える
	* @param bodyID    体の番号
	* @param isTrigger 重なりを知らせるだけにするか
	*
	* [EN]
	* Switches whether a body only reports overlaps.
	*
	* @brief Toggles overlap-only
	* @param bodyID    The body number
	* @param isTrigger Whether it only reports overlaps
	*/
	void Physics::BodyTrigger(ScBodyID bodyID, Bool isTrigger)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();

		/// [JP] 重なり用かを切り替え、層の印も合わせる
		/// [EN] Toggle overlap-only and update the layer mark to match
		bodyInterface.SetIsSensor(physicsBodyID, isTrigger);

		JPH::ObjectLayer layer = bodyInterface.GetObjectLayer(physicsBodyID);
		if (isTrigger)
		{
			layer |= Layers::SENSOR;
		}
		else
		{
			layer = static_cast<JPH::ObjectLayer>(layer & ~Layers::SENSOR);
		}
		bodyInterface.SetObjectLayer(physicsBodyID, layer);

		/// [JP] プログラムで動く重なり用の体は、止まった体とも重なりを調べる
		/// [EN] A code-moved overlap body also checks against still bodies
		Bool collideKinematicVsNonDynamic = bodyInterface.GetMotionType(physicsBodyID) == JPH::EMotionType::Kinematic && isTrigger;

		JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), physicsBodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		lock.GetBody().SetCollideKinematicVsNonDynamic(collideKinematicVsNonDynamic);
	}

	/**
	* [JP]
	* 体があるかを調べる。
	*
	* @brief  体があるか調べる
	* @param  bodyID 体の番号
	* @return あれば true
	*
	* [EN]
	* Checks whether a body exists.
	*
	* @brief  Checks if a body exists
	* @param  bodyID The body number
	* @return True if it exists
	*/
	Bool Physics::BodyExists(ScBodyID bodyID)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return false;
		}

		return joltManager_.BodyInterface().IsAdded(physicsBodyID);
	}

	/**
	* [JP]
	* やわらかい体の今の頂点の位置を取り出す。
	*
	* @brief 頂点の位置を取り出す
	* @param bodyID       体の番号
	* @param outPositions 頂点の位置が入る
	*
	* [EN]
	* Gets the current vertex positions of a soft body.
	*
	* @brief Gets the vertices
	* @param bodyID       The body number
	* @param outPositions Gets the vertex positions
	*/
	void Physics::BodyVertex(ScBodyID bodyID, DynamicArray<Vector3>& outPositions)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		/// [JP] ロックして、やわらかい体の頂点を読む
		/// [EN] Lock the body and read the soft body's vertices
		const JPH::BodyLockInterface& lockInterface = joltManager_.PhysicsSystem().GetBodyLockInterface();

		JPH::BodyLockRead lock(lockInterface, physicsBodyID);
		if (!lock.Succeeded())
		{
			return;
		}

		const JPH::Body& body = lock.GetBody();
		const JPH::SoftBodyMotionProperties* motionProperties = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionPropertiesUnchecked());
		if (!motionProperties)
		{
			return;
		}

		/// [JP] 頂点の位置を世界での位置に直して写す
		/// [EN] Turn each vertex into a world position and copy it
		JPH::RMat44 transform = body.GetCenterOfMassTransform();
		const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& vertices = motionProperties->GetVertices();

		outPositions.clear();
		outPositions.reserve(vertices.size());
		for (const JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
		{
			JPH::RVec3 worldPosition = transform * vertex.mPosition;
			outPositions.push_back(Vector3(static_cast<Float>(worldPosition.GetX()), static_cast<Float>(worldPosition.GetY()), static_cast<Float>(worldPosition.GetZ())));
		}
	}

	/**
	* [JP]
	* 体の持ち主のアクターを返す。
	*
	* @brief  持ち主を返す
	* @param  bodyID 体の番号
	* @return 持ち主のアクター
	*
	* [EN]
	* Returns the actor that owns a body.
	*
	* @brief  Returns the owner
	* @param  bodyID The body number
	* @return The owning actor
	*/
	EntityID Physics::BodyEntityID(ScBodyID bodyID)const
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return EntityID{};
		}

		const JPH::BodyLockInterface& lockInterface = joltManager_.PhysicsSystem().GetBodyLockInterface();

		JPH::BodyLockRead lock(lockInterface, physicsBodyID);
		if (!lock.Succeeded())
		{
			return EntityID{};
		}

		return std::bit_cast<EntityID>(static_cast<Uint64>(lock.GetBody().GetUserData()));
	}

	/**
	* [JP]
	* 世界にある体の番号を全部返す。
	*
	* @brief  体の一覧を返す
	* @return 体の番号の一覧
	*
	* [EN]
	* Returns every body number in the world.
	*
	* @brief  Returns every body
	* @return The body numbers
	*/
	DynamicArray<ScBodyID> Physics::BodyList()const
	{
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
	* [JP]
	* 体に力をかける。重さに合わせて少しずつ速くなる。
	*
	* @brief 力をかける
	* @param bodyID 体の番号
	* @param force  かける力
	*
	* [EN]
	* Pushes a body with a force; it speeds up gradually, depending on its mass.
	*
	* @brief Adds a force
	* @param bodyID The body number
	* @param force  The force
	*/
	void Physics::AddForce(ScBodyID bodyID, const Vector3& force)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		joltManager_.BodyInterface().AddForce(physicsBodyID, JPH::Vec3(force.x, force.y, force.z));
	}

	/**
	* [JP]
	* 体を一瞬でぽんと押す。
	*
	* @brief 一瞬押す
	* @param bodyID  体の番号
	* @param impulse 押す強さ
	*
	* [EN]
	* Gives a body a quick shove.
	*
	* @brief Adds an impulse
	* @param bodyID  The body number
	* @param impulse How hard to shove
	*/
	void Physics::AddImpulse(ScBodyID bodyID, const Vector3& impulse)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();

		/// [JP] 普通の体なら、そのまま押す
		/// [EN] For a normal body, just shove it
		if (bodyInterface.GetBodyType(physicsBodyID) != JPH::EBodyType::SoftBody)
		{
			bodyInterface.AddImpulse(physicsBodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
			return;
		}

		/// [JP] やわらかい体は、頂点全体の重さで割って、全部の頂点に同じ速さを足す
		/// [EN] For a soft body, divide by the total vertex mass and add the same speed to every vertex
		{
			JPH::BodyLockWrite lock(joltManager_.PhysicsSystem().GetBodyLockInterface(), physicsBodyID);
			if (!lock.Succeeded())
			{
				return;
			}

			JPH::SoftBodyMotionProperties* motionProperties = static_cast<JPH::SoftBodyMotionProperties*>(lock.GetBody().GetMotionPropertiesUnchecked());
			if (!motionProperties)
			{
				return;
			}

			JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& vertices = motionProperties->GetVertices();
			Float totalMass = 0.0f;
			for (const JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
			{
				if (vertex.mInvMass > 0.0f)
				{
					totalMass += 1.0f / vertex.mInvMass;
				}
			}

			if (totalMass <= 0.0f)
			{
				return;
			}

			JPH::Vec3 deltaVelocity = JPH::Vec3(impulse.x, impulse.y, impulse.z) / totalMass;
			for (JPH::SoftBodyMotionProperties::Vertex& vertex : vertices)
			{
				if (vertex.mInvMass > 0.0f)
				{
					vertex.mVelocity += deltaVelocity;
				}
			}
		}

		/// [JP] 眠っていたら起こす
		/// [EN] Wake it if it was asleep
		bodyInterface.ActivateBody(physicsBodyID);
	}

	/**
	* [JP]
	* 体を回そうとする力をかける。
	*
	* @brief 回す力をかける
	* @param bodyID 体の番号
	* @param torque 回す力
	*
	* [EN]
	* Applies a turning force to a body.
	*
	* @brief Adds a torque
	* @param bodyID The body number
	* @param torque The turning force
	*/
	void Physics::AddTorque(ScBodyID bodyID, const Vector3& torque)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		joltManager_.BodyInterface().AddTorque(physicsBodyID, JPH::Vec3(torque.x, torque.y, torque.z));
	}

	/**
	* [JP]
	* 体を一瞬でぽんと回す。
	*
	* @brief 一瞬回す
	* @param bodyID         体の番号
	* @param angularImpulse 回す強さ
	*
	* [EN]
	* Gives a body a quick spin.
	*
	* @brief Adds a spin
	* @param bodyID         The body number
	* @param angularImpulse How hard to spin
	*/
	void Physics::AddSpin(ScBodyID bodyID, const Vector3& angularImpulse)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid())
		{
			return;
		}

		joltManager_.BodyInterface().AddAngularImpulse(physicsBodyID, JPH::Vec3(angularImpulse.x, angularImpulse.y, angularImpulse.z));
	}

	/**
	* [JP]
	* プログラムで動かす体を、次の位置と向きまで動かす。
	*
	* @brief 目標まで動かす
	* @param bodyID         体の番号
	* @param targetPosition 目標の位置
	* @param targetRotation 目標の向き
	* @param elapsedTime    たどり着くまでの時間
	*
	* [EN]
	* Moves a code-driven body toward its next position and facing.
	*
	* @brief Moves to a target
	* @param bodyID         The body number
	* @param targetPosition The target position
	* @param targetRotation The target facing
	* @param elapsedTime    How long it has to get there
	*/
	void Physics::MoveTarget(ScBodyID bodyID, const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime)
	{
		const JPH::BodyID physicsBodyID(bodyID);
		if (physicsBodyID.IsInvalid() || elapsedTime <= 0.0f)
		{
			return;
		}

		/// [JP] プログラムで動かす体だけを動かす
		/// [EN] Only code-moved bodies are moved here
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		if (bodyInterface.GetMotionType(physicsBodyID) != JPH::EMotionType::Kinematic)
		{
			return;
		}

		bodyInterface.MoveKinematic(physicsBodyID, JPH::RVec3(targetPosition.x, targetPosition.y, targetPosition.z), JPH::Quat(targetRotation.x, targetRotation.y, targetRotation.z, targetRotation.w).Normalized(), elapsedTime);
	}

	/**
	* [JP]
	* ヒンジのつなぎを作る。
	*
	* @brief  ヒンジを作る
	* @param  bodyA 1つめの体
	* @param  bodyB 2つめの体。無効なら世界につなぐ
	* @param  desc  ヒンジの設定
	* @return つなぎの番号
	*
	* [EN]
	* Makes a hinge joint.
	*
	* @brief  Makes a hinge joint
	* @param  bodyA The first body
	* @param  bodyB The second body; invalid joins to the world
	* @param  desc  The hinge settings
	* @return The joint number
	*/
	ScConstraintHandle Physics::CreateHingeJoint(ScBodyID scBodyA, ScBodyID scBodyB, const HingeJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);

		/// [JP] 自分の体の位置と向きから、つなぐ点と軸を世界の向きに直す
		/// [EN] Turn the joining point and axis into world space using our body's transform
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 bodyPosition = bodyInterface.GetPosition(bodyA);
		JPH::Quat bodyRotation = bodyInterface.GetRotation(bodyA);

		JPH::Vec3 worldPosition(bodyPosition.GetX(), bodyPosition.GetY(), bodyPosition.GetZ());
		JPH::Vec3 worldAnchor = worldPosition + bodyRotation * JPH::Vec3(desc.anchor_.x, desc.anchor_.y, desc.anchor_.z);

		JPH::Vec3 worldAxis = (bodyRotation * JPH::Vec3(desc.axis_.x, desc.axis_.y, desc.axis_.z)).NormalizedOr(JPH::Vec3::sAxisY());

		JPH::Vec3 worldNormal = worldAxis.GetNormalizedPerpendicular();

		/// [JP] ヒンジの設定を作る
		/// [EN] Build the hinge settings
		JPH::HingeConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = settings.mPoint2 = JPH::RVec3(worldAnchor);
		settings.mHingeAxis1 = settings.mHingeAxis2 = worldAxis;
		settings.mNormalAxis1 = settings.mNormalAxis2 = worldNormal;

		/// [JP] 角度の制限を使うなら、度をラジアンに直して入れる
		/// [EN] When limiting the angle, convert degrees to radians
		if (desc.useLimits_)
		{
			settings.mLimitsMin = ToRadians(desc.minAngle_);
			settings.mLimitsMax = ToRadians(desc.maxAngle_);
		}

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [JP]
	* くっつけるつなぎを作る。
	*
	* @brief  くっつけるつなぎを作る
	* @param  bodyA 1つめの体
	* @param  bodyB 2つめの体。無効なら世界につなぐ
	* @param  desc  つなぎの設定
	* @return つなぎの番号
	*
	* [EN]
	* Makes a fixed joint.
	*
	* @brief  Makes a fixed joint
	* @param  bodyA The first body
	* @param  bodyB The second body; invalid joins to the world
	* @param  desc  The joint settings
	* @return The joint number
	*/
	ScConstraintHandle Physics::CreateFixedJoint(ScBodyID scBodyA, ScBodyID scBodyB, const FixedJointDesc&)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);

		/// [JP] 今の位置関係のままくっつける
		/// [EN] Glue them in their current positions
		JPH::FixedConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = true;

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [JP]
	* ばねのつなぎを作る。
	*
	* @brief  ばねを作る
	* @param  bodyA 1つめの体
	* @param  bodyB 2つめの体。無効なら世界につなぐ
	* @param  desc  ばねの設定
	* @return つなぎの番号
	*
	* [EN]
	* Makes a spring joint.
	*
	* @brief  Makes a spring joint
	* @param  bodyA The first body
	* @param  bodyB The second body; invalid joins to the world
	* @param  desc  The spring settings
	* @return The joint number
	*/
	ScConstraintHandle Physics::CreateSpringJoint(ScBodyID scBodyA, ScBodyID scBodyB, const SpringJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);

		/// [JP] 自分のつなぐ点を世界の位置に直す
		/// [EN] Turn our joining point into a world position
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::RVec3 positionA = bodyInterface.GetPosition(bodyA);
		JPH::Quat rotationA = bodyInterface.GetRotation(bodyA);

		JPH::Vec3 worldPositionA(positionA.GetX(), positionA.GetY(), positionA.GetZ());
		JPH::Vec3 anchorSelf = worldPositionA + rotationA * JPH::Vec3(desc.anchor_.x, desc.anchor_.y, desc.anchor_.z);

		/// [JP] 相手がいれば相手の中心、いなければ同じ点につなぐ
		/// [EN] Join to the other body's center, or to the same point if there is none
		JPH::Vec3 anchorOther = anchorSelf;
		if (!bodyB.IsInvalid())
		{
			JPH::RVec3 positionB = bodyInterface.GetPosition(bodyB);
			anchorOther = JPH::Vec3(positionB.GetX(), positionB.GetY(), positionB.GetZ());
		}

		/// [JP] 距離のつなぎの設定を作る
		/// [EN] Build the distance joint settings
		JPH::DistanceConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = JPH::RVec3(anchorOther);
		settings.mPoint2 = JPH::RVec3(anchorSelf);

		/// [JP] 距離が両方 0 なら、今の距離を保つ
		/// [EN] If both distances are 0, keep the current distance
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

		/// [JP] 伸び縮みの硬さと揺れの止まりやすさを決める
		/// [EN] Set the spring stiffness and how quickly bouncing settles
		settings.mLimitsSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, desc.frequency_, desc.damping_);

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [JP]
	* 滑るつなぎを作る。
	*
	* @brief  滑るつなぎを作る
	* @param  bodyA 1つめの体
	* @param  bodyB 2つめの体。無効なら世界につなぐ
	* @param  desc  滑るつなぎの設定
	* @return つなぎの番号
	*
	* [EN]
	* Makes a slider joint.
	*
	* @brief  Makes a slider joint
	* @param  bodyA The first body
	* @param  bodyB The second body; invalid joins to the world
	* @param  desc  The slider settings
	* @return The joint number
	*/
	ScConstraintHandle Physics::CreateSliderJoint(ScBodyID scBodyA, ScBodyID scBodyB, const SliderJointDesc& desc)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);

		/// [JP] 滑る向きを世界の向きに直して、滑るつなぎを作る
		/// [EN] Turn the slide direction into world space and build the slider
		JPH::BodyInterface& bodyInterface = joltManager_.BodyInterface();
		JPH::Quat rotationA = bodyInterface.GetRotation(bodyA);
		JPH::Vec3 worldAxis = (rotationA * JPH::Vec3(desc.axis_.x, desc.axis_.y, desc.axis_.z)).NormalizedOr(JPH::Vec3::sAxisX());

		JPH::SliderConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = true;
		settings.SetSliderAxis(worldAxis);

		/// [JP] 動ける範囲を使うなら入れる
		/// [EN] Set the limits when they are used
		if (desc.useLimits_)
		{
			settings.mLimitsMin = desc.minDistance_;
			settings.mLimitsMax = desc.maxDistance_;
		}

		return CreateConstraint(scBodyA, scBodyB, settings);
	}

	/**
	* [JP]
	* つなぎを消す。
	*
	* @brief つなぎを消す
	* @param handle つなぎの番号
	*
	* [EN]
	* Removes a joint.
	*
	* @brief Removes a joint
	* @param handle The joint number
	*/
	void Physics::DestroyJoint(ScConstraintHandle handle)
	{
		joltManager_.ConstraintPool().Release(joltManager_.PhysicsSystem(), handle);
	}

	/**
	* [JP]
	* つないでいる体が世界にいるかに合わせて、つなぎを効かせたり止めたりする。
	*
	* @brief つなぎの有効・無効を合わせる
	*
	* [EN]
	* Turns each joint on or off to match whether its bodies are in the world.
	*
	* @brief Refreshes the joints
	*/
	void Physics::RefleshJoint()
	{
		joltManager_.ConstraintPool().Refresh();
	}

	/**
	* [JP]
	* まっすぐな光線を飛ばして、最初に当たったものを調べる。
	*
	* @brief  光線を飛ばす
	* @param  origin      飛ばし始める位置
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Casts a straight ray and finds the first thing it hits.
	*
	* @brief  Casts a ray
	* @param  origin      Where the ray starts
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Raycast(const Vector3& origin, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [JP] 向きをそろえて、光線を作る
		/// [EN] Normalize the direction and build the ray
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::RRayCast ray(JPH::RVec3(origin.x, origin.y, origin.z), dir * maxDistance);

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Raycast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体と、選んでいないレイヤーは飛ばす
		/// [EN] Look in nearest order, skipping 2D bodies and layers not asked for
		collector.Sort();

		for (const JPH::RayCastResult& hit : collector.mHits)
		{
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID) & Layers::PLANAR)
			{
				continue;
			}

			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [JP] 当たった位置と面の向きを調べる
			/// [EN] Find the hit position and the surface facing
			JPH::RVec3 hitPosition = ray.GetPointOnRay(hit.mFraction);

			JPH::Vec3 normal = JPH::Vec3::sZero();
			{
				JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
				if (lock.Succeeded())
				{
					normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPosition);
				}
			}

			/// [JP] 結果を書き込んで、記録にも残す
			/// [EN] Write the result and record it too
			outHit.position_ = Vector3(static_cast<Float>(hitPosition.GetX()), static_cast<Float>(hitPosition.GetY()), static_cast<Float>(hitPosition.GetZ()));
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 球を飛ばして、最初に当たったものを調べる。
	*
	* @brief  球を飛ばす
	* @param  origin      飛ばし始める位置
	* @param  radius      球の半径
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Sweeps a sphere and finds the first thing it hits.
	*
	* @brief  Casts a sphere
	* @param  origin      Where it starts
	* @param  radius      The sphere radius
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Spherecast(const Vector3& origin, Float radius, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [JP] 向きをそろえて、飛ばす球を作る
		/// [EN] Normalize the direction and build the sphere to sweep
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::SphereShape sphereShape(radius);
		JPH::RShapeCast shapeCast(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z)), dir * maxDistance);

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Spherecast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
		query.radius_ = radius;
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::ShapeCastSettings settings;

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体と、選んでいないレイヤーは飛ばす
		/// [EN] Look in nearest order, skipping 2D bodies and layers not asked for
		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
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

			/// [JP] 結果を書き込んで、記録にも残す
			/// [EN] Write the result and record it too
			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			outHit.position_ = Vector3(hit.mContactPointOn2.GetX(), hit.mContactPointOn2.GetY(), hit.mContactPointOn2.GetZ());
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 箱を飛ばして、最初に当たったものを調べる。
	*
	* @brief  箱を飛ばす
	* @param  origin      飛ばし始める位置
	* @param  halfExtents 箱の大きさの半分
	* @param  rotation    箱の向き
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Sweeps a box and finds the first thing it hits.
	*
	* @brief  Casts a box
	* @param  origin      Where it starts
	* @param  halfExtents Half the box size
	* @param  rotation    The box facing
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Boxcast(const Vector3& origin, const Vector3& halfExtents, const Quaternion& rotation, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask)const
	{
		/// [JP] 向きをそろえて、飛ばす箱を作る
		/// [EN] Normalize the direction and build the box to sweep
		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::BoxShape boxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), 0.0f);
		JPH::RShapeCast shapeCast(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::RVec3(origin.x, origin.y, origin.z)), dir * maxDistance);

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Boxcast;
		query.origin_ = origin;
		query.direction_ = Vector3(dir.GetX(), dir.GetY(), dir.GetZ());
		query.distance_ = maxDistance;
		query.shape_ = ColliderKind::Box;
		query.dimensions_ = halfExtents;
		query.rotation_ = rotation;
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::ShapeCastSettings settings;

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体と、選んでいないレイヤーは飛ばす
		/// [EN] Look in nearest order, skipping 2D bodies and layers not asked for
		collector.Sort();

		for (const JPH::ShapeCastResult& hit : collector.mHits)
		{
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
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

			/// [JP] 結果を書き込んで、記録にも残す
			/// [EN] Write the result and record it too
			JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();

			outHit.position_ = Vector3(hit.mContactPointOn2.GetX(), hit.mContactPointOn2.GetY(), hit.mContactPointOn2.GetZ());
			outHit.normal_ = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = outHit.position_;
			query.hitNormal_ = outHit.normal_;
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 形を置いて、重なっているアクターを全部探す。
	*
	* @brief  重なっているものを探す
	* @param  kind       形の種類
	* @param  dimensions 形の大きさ
	* @param  position   置く位置
	* @param  rotation   置く向き
	* @param  layerMask  探すレイヤーの組み合わせ
	* @return 重なっているアクターの一覧
	*
	* [EN]
	* Places a shape and finds every actor it overlaps.
	*
	* @brief  Finds overlaps
	* @param  kind       The kind of shape
	* @param  dimensions The shape size
	* @param  position   Where to place it
	* @param  rotation   Which way it faces
	* @param  layerMask  Which layers to look at
	* @return Every overlapping actor
	*/
	DynamicArray<EntityID> Physics::Overlap(OverlapShapeKind kind, const Vector3& dimensions, const Vector3& position, const Quaternion& rotation, Uint32 layerMask)const
	{
		DynamicArray<EntityID> result;

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Overlap;
		query.origin_ = position;
		query.rotation_ = rotation;
		query.dimensions_ = dimensions;
#endif

		/// [JP] 種類に合わせて、調べる形を作る
		/// [EN] Build the shape to check, by kind
		JPH::ShapeRefC queryShape;
		switch (kind)
		{
		case OverlapShapeKind::Sphere:
		{
			queryShape = new JPH::SphereShape(dimensions.x);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Sphere;
#endif
			break;
		}
		case OverlapShapeKind::Box:
		{
			queryShape = new JPH::BoxShape(JPH::Vec3(dimensions.x, dimensions.y, dimensions.z), 0.0f);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Box;
#endif
			break;
		}
		case OverlapShapeKind::Capsule:
		{
			queryShape = new JPH::CapsuleShape(dimensions.y, dimensions.x);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Capsule;
#endif
			break;
		}
		case OverlapShapeKind::Cylinder:
		{
			queryShape = new JPH::CylinderShape(dimensions.y, dimensions.x, 0.0f);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Cylinder;
#endif
			break;
		}
		default:
		{
			return result;
		}
		}

		/// [JP] 形を置いて、重なっているものを全部集める
		/// [EN] Place the shape and collect everything it overlaps
		JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::RVec3(position.x, position.y, position.z));

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CollideShape(queryShape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

		/// [JP] 2D の体と、選んでいないレイヤーは飛ばす
		/// [EN] Skip 2D bodies and layers not asked for
		for (const JPH::CollideShapeResult& hit : collector.mHits)
		{
			if (physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID2) & Layers::PLANAR)
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

			result.push_back(BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
		}

#if SC_DEBUG || SC_DEVELOP
		query.hit_ = !result.empty();
		queryInstance_.Add(query);
#endif

		return result;
	}

	/**
	* [JP]
	* 2D でまっすぐな光線を飛ばして、最初に当たったものを調べる。
	*
	* @brief  2D の光線を飛ばす
	* @param  origin      飛ばし始める位置（ピクセル）
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離（ピクセル）
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Casts a straight 2D ray and finds the first thing it hits.
	*
	* @brief  Casts a 2D ray
	* @param  origin      Where the ray starts, in pixels
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes, in pixels
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Raycast2D(const Vector2& origin, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		/// [JP] 2D は Y を逆にし、ピクセルをメートルに直して光線を作る
		/// [EN] For 2D, flip Y and turn pixels into meters to build the ray
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::RRayCast ray(JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f), dir * (maxDistance / pixelsPerMeter_));

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Raycast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体だけを、選んだレイヤーから探す
		/// [EN] Look in nearest order for 2D bodies only, in the layers asked for
		collector.Sort();

		for (const JPH::RayCastResult& hit : collector.mHits)
		{
			if (!(physicsSystem.GetBodyInterface().GetObjectLayer(hit.mBodyID) & Layers::PLANAR))
			{
				continue;
			}

			if (layerMask != 0xFFFFFFFF)
			{
				Actor actor = WorldAccessor::Make(world_, BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber()));
				if (actor && (layerMask & (1u << actor.Layer())) == 0)
				{
					continue;
				}
			}

			/// [JP] 当たった位置と面の向きを調べる
			/// [EN] Find the hit position and the surface facing
			JPH::RVec3 hitPosition = ray.GetPointOnRay(hit.mFraction);

			JPH::Vec3 normal = JPH::Vec3::sZero();
			{
				JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
				if (lock.Succeeded())
				{
					normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPosition);
				}
			}

			/// [JP] メートルをピクセルに戻し、Y も戻して書き込む
			/// [EN] Turn meters back into pixels, flip Y back and write the result
			outHit.position_ = Vector2(static_cast<Float>(hitPosition.GetX()) * pixelsPerMeter_, -static_cast<Float>(hitPosition.GetY()) * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 2D で円を飛ばして、最初に当たったものを調べる。
	*
	* @brief  2D の円を飛ばす
	* @param  origin      飛ばし始める位置（ピクセル）
	* @param  radius      円の半径（ピクセル）
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離（ピクセル）
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Sweeps a 2D circle and finds the first thing it hits.
	*
	* @brief  Casts a 2D circle
	* @param  origin      Where it starts, in pixels
	* @param  radius      The circle radius, in pixels
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes, in pixels
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Circlecast2D(const Vector2& origin, Float radius, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		/// [JP] 2D は Y を逆にし、ピクセルをメートルに直して飛ばす円を作る
		/// [EN] For 2D, flip Y and turn pixels into meters to build the circle
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::SphereShape sphereShape(radius / pixelsPerMeter_);
		JPH::RShapeCast shapeCast(&sphereShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f)), dir * (maxDistance / pixelsPerMeter_));

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Circlecast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
		query.radius_ = radius;
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::ShapeCastSettings settings;
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体だけを、選んだレイヤーから探す
		/// [EN] Look in nearest order for 2D bodies only, in the layers asked for
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

			/// [JP] メートルをピクセルに戻し、Y も戻して書き込む
			/// [EN] Turn meters back into pixels, flip Y back and write the result
			outHit.position_ = Vector2(hit.mContactPointOn2.GetX() * pixelsPerMeter_, -hit.mContactPointOn2.GetY() * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 2D で四角を飛ばして、最初に当たったものを調べる。
	*
	* @brief  2D の四角を飛ばす
	* @param  origin      飛ばし始める位置（ピクセル）
	* @param  halfExtents 四角の大きさの半分（ピクセル）
	* @param  rotation    四角の向き（ラジアン）
	* @param  direction   飛ばす向き
	* @param  maxDistance 飛ばす距離（ピクセル）
	* @param  outHit      当たったものの情報が入る
	* @param  layerMask   当てるレイヤーの組み合わせ
	* @return 当たったら true
	*
	* [EN]
	* Sweeps a 2D rectangle and finds the first thing it hits.
	*
	* @brief  Casts a 2D rectangle
	* @param  origin      Where it starts, in pixels
	* @param  halfExtents Half the rectangle size, in pixels
	* @param  rotation    The rectangle angle, in radians
	* @param  direction   Which way it goes
	* @param  maxDistance How far it goes, in pixels
	* @param  outHit      Gets what was hit
	* @param  layerMask   Which layers it can hit
	* @return True if it hit something
	*/
	Bool Physics::Boxcast2D(const Vector2& origin, const Vector2& halfExtents, Float rotation, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask)const
	{
		/// [JP] 2D は Y を逆にし、ピクセルをメートルに直して飛ばす四角を作る
		/// [EN] For 2D, flip Y and turn pixels into meters to build the rectangle
		JPH::Vec3 dir(direction.x, -direction.y, 0.0f);
		if (dir.LengthSq() > 0.0f)
		{
			dir = dir.Normalized();
		}

		JPH::BoxShape boxShape(JPH::Vec3(halfExtents.x / pixelsPerMeter_, halfExtents.y / pixelsPerMeter_, 0.5f), 0.0f);
		JPH::RShapeCast shapeCast(&boxShape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sRotationTranslation(JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -ToRadians(rotation)), JPH::RVec3(origin.x / pixelsPerMeter_, -origin.y / pixelsPerMeter_, 0.0f)), dir * (maxDistance / pixelsPerMeter_));

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Boxcast2D;
		query.origin_ = Vector3(origin.x, origin.y, 0.0f);
		query.direction_ = Vector3(dir.GetX(), -dir.GetY(), 0.0f);
		query.distance_ = maxDistance;
		query.shape_ = ColliderKind::Rect;
		query.dimensions_ = Vector3(halfExtents.x, halfExtents.y, 0.0f);
		query.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, ToRadians(rotation));
#endif

		/// [JP] 当たったものを全部集める
		/// [EN] Collect everything it hits
		JPH::ShapeCastSettings settings;
		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

		if (!collector.HadHit())
		{
#if SC_DEBUG || SC_DEVELOP
			queryInstance_.Add(query);
#endif
			return false;
		}

		/// [JP] 近い順に見て、2D の体だけを、選んだレイヤーから探す
		/// [EN] Look in nearest order for 2D bodies only, in the layers asked for
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

			/// [JP] メートルをピクセルに戻し、Y も戻して書き込む
			/// [EN] Turn meters back into pixels, flip Y back and write the result
			outHit.position_ = Vector2(hit.mContactPointOn2.GetX() * pixelsPerMeter_, -hit.mContactPointOn2.GetY() * pixelsPerMeter_);
			outHit.normal_ = Vector2(normal.GetX(), -normal.GetY());
			outHit.distance_ = hit.mFraction * maxDistance;
			outHit.entityID_ = BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber());

#if SC_DEBUG || SC_DEVELOP
			query.hit_ = true;
			query.hitPoint_ = Vector3(outHit.position_.x, outHit.position_.y, 0.0f);
			query.hitNormal_ = Vector3(outHit.normal_.x, outHit.normal_.y, 0.0f);
			query.hitDistance_ = outHit.distance_;
			queryInstance_.Add(query);
#endif
			return true;
		}

#if SC_DEBUG || SC_DEVELOP
		queryInstance_.Add(query);
#endif
		return false;
	}

	/**
	* [JP]
	* 2D で形を置いて、重なっているアクターを全部探す。
	*
	* @brief  2D で重なっているものを探す
	* @param  kind       形の種類
	* @param  dimensions 形の大きさ（ピクセル）
	* @param  position   置く位置（ピクセル）
	* @param  rotation   置く向き（ラジアン）
	* @param  layerMask  探すレイヤーの組み合わせ
	* @return 重なっているアクターの一覧
	*
	* [EN]
	* Places a 2D shape and finds every actor it overlaps.
	*
	* @brief  Finds 2D overlaps
	* @param  kind       The kind of shape
	* @param  dimensions The shape size, in pixels
	* @param  position   Where to place it, in pixels
	* @param  rotation   The angle, in radians
	* @param  layerMask  Which layers to look at
	* @return Every overlapping actor
	*/
	DynamicArray<EntityID> Physics::Overlap2D(OverlapShapeKind kind, const Vector2& dimensions, const Vector2& position, Float rotation, Uint32 layerMask)const
	{
		DynamicArray<EntityID> result;

/// [JP] 調べたことを画面に描けるように記録を用意する（Debug と Develop だけ）
/// [EN] Prepare a record so the check can be drawn on screen (Debug and Develop only)
#if SC_DEBUG || SC_DEVELOP
		QueryDesc query;
		query.kind_ = QueryKind::Overlap2D;
		query.origin_ = Vector3(position.x, position.y, 0.0f);
		query.rotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, ToRadians(rotation));
		query.dimensions_ = Vector3(dimensions.x, dimensions.y, 0.0f);
#endif

		/// [JP] 種類に合わせて、ピクセルをメートルに直した形を作る
		/// [EN] Build the shape by kind, turning pixels into meters
		JPH::ShapeRefC queryShape;
		switch (kind)
		{
		case OverlapShapeKind::Circle:
		{
			queryShape = new JPH::SphereShape(dimensions.x / pixelsPerMeter_);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Circle;
#endif
			break;
		}
		case OverlapShapeKind::Rect:
		{
			queryShape = new JPH::BoxShape(JPH::Vec3(dimensions.x / pixelsPerMeter_, dimensions.y / pixelsPerMeter_, 0.5f), 0.0f);
#if SC_DEBUG || SC_DEVELOP
			query.shape_ = ColliderKind::Rect;
#endif
			break;
		}
		default:
		{
			return result;
		}
		}

		/// [JP] 形を置いて、重なっているものを全部集める
		/// [EN] Place the shape and collect everything it overlaps
		JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -ToRadians(rotation)), JPH::RVec3(position.x / pixelsPerMeter_, -position.y / pixelsPerMeter_, 0.0f));

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();

		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		physicsSystem.GetNarrowPhaseQuery().CollideShape(queryShape.GetPtr(), JPH::Vec3::sReplicate(1.0f), transform, JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

		/// [JP] 2D の体だけを、選んだレイヤーから探す
		/// [EN] Keep only 2D bodies in the layers asked for
		for (const JPH::CollideShapeResult& hit : collector.mHits)
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

			result.push_back(BodyEntityID(hit.mBodyID2.GetIndexAndSequenceNumber()));
		}

#if SC_DEBUG || SC_DEVELOP
		query.hit_ = !result.empty();
		queryInstance_.Add(query);
#endif

		return result;
	}

	/**
	* [JP]
	* 2つの体をつなぐつなぎを作る本体。
	*
	* @brief  つなぎを作る
	* @param  bodyA    1つめの体
	* @param  bodyB    2つめの体。無効なら世界につなぐ
	* @param  settings つなぎの設定
	* @return つなぎの番号
	*
	* [EN]
	* The shared code that makes a joint between two bodies.
	*
	* @brief  Makes a joint
	* @param  bodyA    The first body
	* @param  bodyB    The second body; invalid joins to the world
	* @param  settings The joint settings
	* @return The joint number
	*/
	ScConstraintHandle Physics::CreateConstraint(ScBodyID scBodyA, ScBodyID scBodyB, const JPH::TwoBodyConstraintSettings& settings)
	{
		const JPH::BodyID bodyA(scBodyA);
		const JPH::BodyID bodyB(scBodyB);
		if (bodyA.IsInvalid())
		{
			return ScConstraintHandle::null();
		}

		JPH::PhysicsSystem& physicsSystem = joltManager_.PhysicsSystem();
		const JPH::BodyLockInterface& lockInterface = physicsSystem.GetBodyLockInterface();

		JPH::Constraint* constraint = nullptr;

		/// [JP] 相手がいなければ世界につなぎ、いれば2つの体をロックしてつなぐ
		/// [EN] Join to the world if there is no other body, otherwise lock both bodies and join them
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

		/// [JP] できたつなぎを物理エンジンに入れて、番号をつけて持つ
		/// [EN] Add the new joint to the physics engine and keep it under a number
		return joltManager_.ConstraintPool().Add(physicsSystem, constraint);
	}

}
