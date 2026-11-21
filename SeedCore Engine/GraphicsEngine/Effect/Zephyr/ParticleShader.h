#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/DynamicPipeline.h>

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

	struct ParticleEmitter
	{
		Float delta_ = 0.0f;
		Float age_ = 0.0f;
		Uint32 simSpace_ = 0;
		Uint32 particleEmitterPadding0_;
		Matrix emitterToWorld_ = Matrix::Identity;
		Matrix worldToEmitter_ = Matrix::Identity;
	};
	SC_STATIC_ASSERT(ParticleEmitter, 144, "Effect/Zephyr/Particle.hlsli");

	struct ParticleEmitterStructuredBuffer
	{
		Uint32 particles_ = 0;
		Uint32 base_ = 0;
		Uint32 capacity_ = 0;
		Uint32 aliveRead_ = 0;
		Uint32 aliveWrite_ = 0;
		Uint32 parameters_ = 0;
		Uint32 spawnCount_ = 0;
		Uint32 particleEmitterStructuredBufferPadding0_;
		ParticleEmitter emitter_;
	};
	SC_STATIC_ASSERT(ParticleEmitterStructuredBuffer, 176, "Effect/Zephyr/ParticlePool.hlsli");

	struct ParticleCounterStructuredBuffer
	{
		Uint32 aliveCount_ = 0;
		Uint32 deadCount_ = 0;
		Uint32 aliveWriteCount_ = 0;
		Uint32 particleCountersStructuredBufferPadding0_;
	};
	SC_STATIC_ASSERT(ParticleCounterStructuredBuffer, 16, "Effect/Zephyr/ParticlePool.hlsli");

	struct ParticleResetDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Effect/Zephyr/ParticleResetCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ParticleBeginDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Effect/Zephyr/ParticleBeginCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 256;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ParticleDrivenDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Effect/Zephyr/ParticleDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 256;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}