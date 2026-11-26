#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>

namespace JPH
{
	class PhysicsSystem;
}

namespace SeedCore
{
	/// [JP] 体の層の番号。動き方と、エディタで決めたレイヤーを1つの数にまとめる
	/// [EN] Body layer numbers; they pack how a body moves and its editor layer into one number
	namespace Layers
	{
		/// [JP] 動かない体
		/// [EN] A still body
		SC_CONSTEX ScObjectLayer STATIC = 0;

		/// [JP] プログラムで動かす体
		/// [EN] A body moved by code
		SC_CONSTEX ScObjectLayer KINEMATIC = 1;

		/// [JP] 重さや力で動く体
		/// [EN] A body moved by weight and forces
		SC_CONSTEX ScObjectLayer DYNAMIC = 2;

		/// [JP] 動き方を入れておくビットの数
		/// [EN] How many bits hold the way it moves
		SC_CONSTEX Uint32 MOTION_TYPE_BITS = 2;

		/// [JP] 重なりを知らせるだけの体につける印
		/// [EN] The mark for a body that only reports overlaps
		SC_CONSTEX ScObjectLayer SENSOR = static_cast<ScObjectLayer>(1u << 14);

		/// [JP] 2D の体につける印。2D と 3D はぶつからない
		/// [EN] The mark for a 2D body; 2D and 3D never hit each other
		SC_CONSTEX ScObjectLayer PLANAR = static_cast<ScObjectLayer>(1u << 15);

		/**
		* [JP]
		* 動き方とレイヤーを、1つの層の番号にまとめる。
		*
		* @brief  層の番号にまとめる
		* @param  motionType 動き方
		* @param  userLayer  エディタで決めたレイヤー
		* @return まとめた層の番号
		*
		* [EN]
		* Packs the way it moves and its layer into one layer number.
		*
		* @brief  Packs a layer number
		* @param  motionType How it moves
		* @param  userLayer  The editor layer
		* @return The packed layer number
		*/
		ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer);

		/**
		* [JP]
		* 層の番号から、動き方だけを取り出す。
		*
		* @brief  動き方を取り出す
		* @param  objectLayer 層の番号
		* @return 動き方
		*
		* [EN]
		* Takes just the way it moves out of a layer number.
		*
		* @brief  Gets the way it moves
		* @param  objectLayer The layer number
		* @return How it moves
		*/
		ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer);

		/**
		* [JP]
		* 層の番号から、エディタで決めたレイヤーだけを取り出す。
		*
		* @brief  レイヤーを取り出す
		* @param  objectLayer 層の番号
		* @return エディタで決めたレイヤー
		*
		* [EN]
		* Takes just the editor layer out of a layer number.
		*
		* @brief  Gets the editor layer
		* @param  objectLayer The layer number
		* @return The editor layer
		*/
		Size UnpackUserLayer(ScObjectLayer objectLayer);
	}

	/// [JP] 大まかな当たり判定で使う層の番号
	/// [EN] Layer numbers for the rough first collision check
	namespace BPLayers
	{
		/// [JP] 動かない体の層
		/// [EN] The layer for still bodies
		SC_CONSTEX ScBroadPhaseLayer STATIC = 0;

		/// [JP] 動く体の層
		/// [EN] The layer for moving bodies
		SC_CONSTEX ScBroadPhaseLayer DYNAMIC = 1;

		/// [JP] 重なりを知らせるだけの体の層
		/// [EN] The layer for overlap-only bodies
		SC_CONSTEX ScBroadPhaseLayer SENSOR = 2;

		/// [JP] 層の数
		/// [EN] How many layers there are
		SC_CONSTEX Uint32 COUNT = 3;
	}

	/**
	* [JP]
	* 何と何がぶつかるかを、物理エンジンに教える係。
	*
	* @brief ぶつかる組み合わせを教える係
	*
	* [EN]
	* Tells the physics engine which things can hit which.
	*
	* @brief Sets up which things hit
	*/
	class JoltLayerFilters final
	{
	public:
		/**
		* [JP]
		* 中身を作る。
		*
		* @brief 中身を作る
		*
		* [EN]
		* Makes the inner parts.
		*
		* @brief Makes the inner parts
		*/
		JoltLayerFilters();

		/**
		* [JP]
		* 中身を片付ける。
		*
		* @brief 中身を片付ける
		*
		* [EN]
		* Cleans up the inner parts.
		*
		* @brief Cleans up the inner parts
		*/
		~JoltLayerFilters();

		JoltLayerFilters(const JoltLayerFilters&) = delete;
		JoltLayerFilters& operator=(const JoltLayerFilters&) = delete;

	public:
		/**
		* [JP]
		* ぶつかる組み合わせを使って、物理エンジンを始める。
		*
		* @brief 物理エンジンを始める
		* @param physicsSystem 始める物理エンジン
		*
		* [EN]
		* Starts the physics engine using these hit rules.
		*
		* @brief Starts the physics engine
		* @param physicsSystem The physics engine to start
		*/
		void Initialize(JPH::PhysicsSystem& physicsSystem);

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};

	/**
	* [JP]
	* Rigidbody の種類を、物理エンジンの動き方に直す。
	*
	* @brief  動き方に直す
	* @param  bodyType Rigidbody の種類
	* @return 動き方
	*
	* [EN]
	* Turns a Rigidbody type into the physics engine's way of moving.
	*
	* @brief  Turns it into a motion type
	* @param  bodyType The Rigidbody type
	* @return How it moves
	*/
	ScMotionType ToMotionType(Rigidbody::BodyType bodyType);

	/**
	* [JP]
	* Rigidbody の種類とレイヤーから、層の番号を作る。
	*
	* @brief  層の番号を作る
	* @param  bodyType  Rigidbody の種類
	* @param  userLayer エディタで決めたレイヤー
	* @return 層の番号
	*
	* [EN]
	* Makes a layer number from a Rigidbody type and a layer.
	*
	* @brief  Makes a layer number
	* @param  bodyType  The Rigidbody type
	* @param  userLayer The editor layer
	* @return The layer number
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType, Size userLayer);

	/**
	* [JP]
	* Rigidbody の種類と、いつものレイヤーから層の番号を作る。
	*
	* @brief  層の番号を作る
	* @param  bodyType Rigidbody の種類
	* @return 層の番号
	*
	* [EN]
	* Makes a layer number from a Rigidbody type and the default layer.
	*
	* @brief  Makes a layer number
	* @param  bodyType The Rigidbody type
	* @return The layer number
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType);
}
