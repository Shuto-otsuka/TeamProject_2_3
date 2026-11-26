#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	class Crister;

	/**
	* [JP]
	* アクターにつけると、メッシュが布やゴムのようにやわらかく動く部品。
	* 形は同じアクターの Mesh から作る。
	*
	* @brief やわらかい体の部品
	*
	* [EN]
	* Put it on an actor to make its mesh move softly, like cloth or rubber.
	* The shape is built from the Mesh on the same actor.
	*
	* @brief A soft body part
	*/
	class SEEDCORE_API Softbody :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [JP] 重さ
		/// [EN] The mass
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("質量", 0.001f, 100.0f)
		Float mass_ = 1.0f;

		/// [JP] 面の広がりにくさ
		/// [EN] How hard the surface is to stretch
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("面積弾性力", 0.0f, 1.0f)
		Float areaStiffness_ = 0.5f;

		/// [JP] 体積の変わりにくさ
		/// [EN] How hard the volume is to change
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("体積弾性力", 0.0f, 1.0f)
		Float volumeStiffness_ = 0.5f;

		/// [JP] 動きの止まりやすさ
		/// [EN] How quickly its movement slows down
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("抵抗力", 0.0f, 1.0f)
		Float damping_ = 0.5f;

		/// [JP] 引っ張ったときの細くなりやすさ
		/// [EN] How much it thins when pulled
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("ポアソン比", 0.0f, 0.5f)
		Float poissonRatio_ = 0.3f;

		/// [JP] 元の形から離れてよい距離
		/// [EN] How far it may drift from its original shape
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("最大許容距離", 0.0f, 10.0f)
		Float maxDistance_ = 0.0f;

		/// [JP] 辺の伸びにくさ
		/// [EN] How hard the edges are to stretch
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("辺補強係数", 0.0f, 1.0f)
		Float edgeStiffness_ = 0.5f;

		/// [JP] 曲がりにくさ
		/// [EN] How hard it is to bend
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("曲耐性", 0.0f, 1.0f)
		Float bendStiffness_ = 0.5f;

		/// [JP] 中から押す力（ふくらみ）
		/// [EN] How hard it pushes out from inside
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("内部圧力", -10.0f, 10.0f)
		Float pressure_ = 0.0f;

		/// [JP] すべりにくさ
		/// [EN] How grippy it is
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("摩擦係数", 0.0f, 1.0f)
		Float friction_ = 0.2f;

		/// [JP] はね返りやすさ
		/// [EN] How bouncy it is
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("反発係数", 0.0f, 1.0f)
		Float restitution_ = 0.5f;

		/// [JP] 重力を受けるか
		/// [EN] Whether gravity pulls it
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("重力")
		Bool useGravity_ = true;

		/// [JP] 重力のかかり方の倍率
		/// [EN] How strongly gravity pulls it
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_CONDITION(useGravity_)
		SC_REFLECTION_FIELD_EX("重力倍率")
		Float gravityScale_ = 1.0f;

		/// [JP] 1回の更新を何回に分けて計算するか
		/// [EN] How many smaller steps each update is split into
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("サブステップ数", 1, 20)
		Int subSteps_ = 4;

		/// [JP] 形を整える計算のくり返し回数
		/// [EN] How many times the shape is solved each step
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("反復回数", 1, 10)
		Int iterationCount_ = 4;

	public:
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
		void OnAwake();

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
		void OnFixedTick(Float elapsedTime);

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
		void OnDestroy();

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
		void OnRevision();

	public:
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
		void Dispatch();

	public:
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
		void AddForce(const Vector3& force);

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
		void AddImpulse(const Vector3& impulse);

	public:
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
		ScBodyID BodyID()const;

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
		const DynamicArray<Vector3>& BodyVertex()const;

	private:
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
		void Build(const Crister& crister);

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
		Bool Pending()const;

	private:
		/// [JP] 物理の体の番号
		/// [EN] The physics body number
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [JP] アクターから見た、今の頂点の位置
		/// [EN] The current vertex positions, relative to the actor
		DynamicArray<Vector3> vertexPositions_;

		/// [JP] まだ体を作れていないか
		/// [EN] Whether the body still needs to be built
		Bool pending_ = true;
	};
	REGISTER_COMPONENT(Softbody, "Physics");
}
