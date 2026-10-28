#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>

namespace JPH
{
	class PhysicsSystem;
}

namespace SeedCore
{
	namespace Layers
	{
		SC_CONSTEX ScObjectLayer STATIC = 0;
		SC_CONSTEX ScObjectLayer KINEMATIC = 1;
		SC_CONSTEX ScObjectLayer DYNAMIC = 2;

		/// [EN] Number of low bits reserved for the motion type.
		/// [JP] 運動タイプ用に確保する下位ビット数。
		SC_CONSTEX Uint32 MOTION_TYPE_BITS = 2;

		/// [EN] Marks a trigger body.
		/// [JP] トリガーボディを示すビット。
		SC_CONSTEX ScObjectLayer SENSOR = static_cast<ScObjectLayer>(1u << 14);

		/// [EN] Marks a body simulated on the 2D canvas.
		/// [JP] 2D Canvas 上でシミュレートするボディを示すビット。
		SC_CONSTEX ScObjectLayer PLANAR = static_cast<ScObjectLayer>(1u << 15);

		ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer);

		ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer);

		Size UnpackUserLayer(ScObjectLayer objectLayer);
	}

	namespace BPLayers
	{
		SC_CONSTEX ScBroadPhaseLayer STATIC = 0;
		SC_CONSTEX ScBroadPhaseLayer DYNAMIC = 1;
		SC_CONSTEX ScBroadPhaseLayer SENSOR = 2;
		SC_CONSTEX Uint32 COUNT = 3;
	}

	/**
	* [EN]
	* Owns Jolt's broad-phase and object-layer filter implementations and
	* uses them to initialize a physics system.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のブロードフェーズ・オブジェクトレイヤーフィルター実装を所有し、
	* それらを使って物理システムを初期化する。
	*/
	class JoltLayerFilters final
	{
	public:
		JoltLayerFilters();
		~JoltLayerFilters();

		JoltLayerFilters(const JoltLayerFilters&) = delete;
		JoltLayerFilters& operator=(const JoltLayerFilters&) = delete;

	public:
		void Initialize(JPH::PhysicsSystem& physicsSystem);

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};

	ScMotionType ToMotionType(Rigidbody::BodyType bodyType);

	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType, Size userLayer);

	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType);
}
