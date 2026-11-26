#include <PhysicsEngine/Softbody/Softbody.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerExtractor.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	/**
	* [JP]
	* メッシュが選ばれていれば、体を作る準備をする。
	*
	* @brief 作る準備をする
	*
	* [EN]
	* If a mesh is chosen, gets ready to build the body.
	*
	* @brief Gets ready to build
	*/
	void Softbody::OnAwake()
	{
		const Mesh* mesh = GetActor().GetComponent<Mesh>();
		pending_ = mesh != nullptr && mesh->meshID_ != 0;
	}

	/**
	* [JP]
	* 体の頂点の位置を、アクターから見た位置に直して覚える。
	*
	* @brief 頂点の位置を写す
	* @param elapsedTime 前の物理の更新からの時間
	*
	* [EN]
	* Reads the body's vertex positions and stores them relative to the actor.
	*
	* @brief Copies the vertex positions
	* @param elapsedTime Time since the last physics update
	*/
	void Softbody::OnFixedTick(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 頂点の位置を取り、アクターから見た位置に直す
		/// [EN] Read the vertex positions and make them relative to the actor
		GetActor().GetService<Physics>()->BodyVertex(bodyID_, vertexPositions_);

		Matrix inverseWorld = GetActor().WorldMatrix().Invert();
		for (Vector3& position : vertexPositions_)
		{
			position = Vector3::TransformPosition(position, inverseWorld);
		}
	}

	/**
	* [JP]
	* 体を消して、作り直せるようにする。
	*
	* @brief 体を消す
	*
	* [EN]
	* Removes the body so it can be built again.
	*
	* @brief Removes the body
	*/
	void Softbody::OnDestroy()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 体を消して、メッシュがあればまた作れるようにする
		/// [EN] Remove the body and allow it to be built again if there is a mesh
		GetActor().GetService<Physics>()->DestroyBody(bodyID_);

		bodyID_ = ScInvalidBodyID;
		vertexPositions_.clear();

		const Mesh* mesh = GetActor().GetComponent<Mesh>();
		pending_ = mesh != nullptr && mesh->meshID_ != 0;
	}

	/**
	* [JP]
	* 設定が変わったので、体を作り直す。
	*
	* @brief 体を作り直す
	*
	* [EN]
	* The settings changed, so the body is made again.
	*
	* @brief Remakes the body
	*/
	void Softbody::OnRevision()
	{
		OnDestroy();
		OnAwake();
	}

	/**
	* [JP]
	* 体を作り直す。
	*
	* @brief 体を作り直す
	*
	* [EN]
	* Remakes the body.
	*
	* @brief Remakes the body
	*/
	void Softbody::Dispatch()
	{
		OnRevision();
	}

	/**
	* [JP]
	* 体に力をかける。
	*
	* @brief 力をかける
	* @param force かける力
	*
	* [EN]
	* Pushes the body with a force.
	*
	* @brief Adds a force
	* @param force The force
	*/
	void Softbody::AddForce(const Vector3& force)
	{
		GetActor().GetService<Physics>()->AddForce(bodyID_, force);
	}

	/**
	* [JP]
	* 体を一瞬でぽんと押す。
	*
	* @brief 一瞬押す
	* @param impulse 押す強さ
	*
	* [EN]
	* Gives the body a quick shove.
	*
	* @brief Adds an impulse
	* @param impulse How hard to shove
	*/
	void Softbody::AddImpulse(const Vector3& impulse)
	{
		GetActor().GetService<Physics>()->AddImpulse(bodyID_, impulse);
	}

	/**
	* [JP]
	* 物理の体の番号を返す。
	*
	* @brief  体の番号を返す
	* @return 体の番号。まだなければ無効な番号
	*
	* [EN]
	* Returns the physics body number.
	*
	* @brief  Returns the body number
	* @return The body number, or the invalid number if there is none yet
	*/
	ScBodyID Softbody::BodyID()const
	{
		return bodyID_;
	}

	/**
	* [JP]
	* アクターから見た、今の頂点の位置を返す。
	*
	* @brief  頂点の位置を返す
	* @return 頂点の位置の一覧
	*
	* [EN]
	* Returns the current vertex positions relative to the actor.
	*
	* @brief  Returns the vertex positions
	* @return The vertex positions
	*/
	const DynamicArray<Vector3>& Softbody::BodyVertex()const
	{
		return vertexPositions_;
	}

	/**
	* [JP]
	* 読み込んだモデルから、やわらかい体を作る。
	*
	* @brief 体を作る
	* @param crister 形に使うモデル
	*
	* [EN]
	* Builds the soft body from the loaded model.
	*
	* @brief Builds the body
	* @param crister The model to build from
	*/
	void Softbody::Build(const Crister& crister)
	{
		Actor actor = GetActor();

		/// [JP] モデルから、一番粗い頂点と三角形を取り出す
		/// [EN] Take the coarsest vertices and triangles from the model
		DynamicArray<Vertex> vertices;
		SoftbodyDesc desc;

		CristerExtractor extractor;
		if (!extractor.CoarsestVertices(crister, vertices, desc.indices_))
		{
			return;
		}

		desc.positions_.reserve(vertices.size());
		for (const Vertex& vertex : vertices)
		{
			desc.positions_.push_back(vertex.position_);
		}

		/// [JP] 層と、伸び・ゆがみ・曲がりのやわらかさを決める。硬さが 1 で一番硬い
		/// [EN] Set the layer and how soft stretching, shearing and bending are; a stiffness of 1 is the stiffest
		desc.layer_ = Layers::Pack(Layers::DYNAMIC, actor.Layer());

		desc.edgeCompliance_ = (1.0f - Clamp(edgeStiffness_, 0.0f, 1.0f)) * 1.0e-4f;
		desc.shearCompliance_ = (1.0f - Clamp(areaStiffness_, 0.0f, 1.0f)) * 1.0e-4f;
		desc.bendCompliance_ = (1.0f - Clamp(bendStiffness_, 0.0f, 1.0f)) * 1.0e-4f;

		/// [JP] 動きの設定をまとめる
		/// [EN] Gather the motion settings
		desc.numIterations_ = static_cast<Uint32>(iterationCount_);
		desc.linearDamping_ = damping_;
		desc.pressure_ = pressure_;
		desc.friction_ = friction_;
		desc.restitution_ = restitution_;
		desc.gravityFactor_ = useGravity_ ? gravityScale_ : 0.0f;

		desc.userData_ = ActorAccessor::GetEntity(actor).GetID();

		/// [JP] 親から見た位置と向きを、世界での位置と向きに直す
		/// [EN] Turn the parent-relative position and facing into world ones
		const Position* position = actor.GetComponent<Position>();
		const Rotation* rotation = actor.GetComponent<Rotation>();

		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

		desc.position_ = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
		desc.rotation_ = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;

		/// [JP] 体を作れたら、準備完了にする
		/// [EN] Once the body is made, mark it as ready
		bodyID_ = actor.GetService<Physics>()->CreateSoftbody(desc);
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		pending_ = false;
	}

	/**
	* [JP]
	* まだ体を作れていないかを返す。
	*
	* @brief  作る待ちか調べる
	* @return まだなら true
	*
	* [EN]
	* Returns whether the body still needs to be built.
	*
	* @brief  Checks if building is pending
	* @return True if it is still pending
	*/
	Bool Softbody::Pending()const
	{
		return pending_;
	}
}
