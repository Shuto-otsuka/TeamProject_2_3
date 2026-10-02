#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	struct ParticleSeed
	{
		Vector3 position_ = { 0.0f,0.0f,0.0f };
		Quaternion rotation_ = { 0.0f,0.0f,0.0f,1.0f };

		Vector3 liveVelocity_ = { 0.0f,0.0f,0.0f };
		Vector3 baseVelocity_ = { 0.0f,0.0f,0.0f };

		Vector3 liveAngularVelocity_ = { 0.0f,0.0f,0.0f };
		Vector3 baseAngularVelocity_ = { 0.0f,0.0f,0.0f };

		Color liveColor_ = { 1.0f,1.0f,1.0f,1.0f };
		Color baseColor_ = { 1.0f,1.0f,1.0f,1.0f };

		Vector3 liveSize_ = { 0.5f,0.5f,0.5f };
		Vector3 baseSize_ = { 0.5f,0.5f,0.5f };

		Float age_ = 0.0f;
		Float lifetime_ = 1.0f;
		Uint32 seed_ = 0;
	};
	SC_STATIC_ASSERT(ParticleSeed, 144, "Effect/Zephyr/Particle.hlsli");

	struct ParticleCounters
	{
		Uint32 aliveCount_ = 0;
		Uint32 deadCount_ = 0;
		Uint32 aliveWriteCount_ = 0;
		Uint32 particleCountersPadding0_;
	};
	SC_STATIC_ASSERT(ParticleCounters, 16, "Effect/Zephyr/ParticlePool.hlsli");
}