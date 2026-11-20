#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Effect/Zephyr/ParticleShader.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class ParticleFeature :public RenderFeature<ParticleFeature>
	{
	private:
		SC_CONSTEX Uint32 sharedCapacity_ = 65536;

		SC_CONSTEX Uint32 sharedThreshold_ = 4096;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Emitters, RenderResourceDesc::FixedBuffer(sizeof(ParticleEmitterStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Parameters, RenderResourceDesc::FixedBuffer(sizeof(Vector4), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Seeds, RenderResourceDesc::DynamicBuffer(sizeof(ParticleSeed)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Dead, RenderResourceDesc::DynamicBuffer(sizeof(Uint32)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Alive, RenderResourceDesc::DynamicBuffer(sizeof(Uint32)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Counters, RenderResourceDesc::DynamicBuffer(sizeof(ParticleCounterStructuredBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Offsets, RenderResourceDesc::DynamicBuffer(sizeof(Uint32)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(SimulateArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 3), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(QuadOffsets, RenderResourceDesc::DynamicBuffer(sizeof(Uint32)), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(QuadArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 3), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(QuadFallbackArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 4), RenderResourceScope::Frame, RenderResourceLifetime::Persistent)
		);

	public:
		struct UpdateStage;

		struct DrawStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<ParticleResetDescription> resetPipeline_;

		ComputePipeline<ParticleBeginDescription> beginPipeline_;

		ComputePipeline<ParticleDrivenDescription> drivenPipeline_;

		DynamicPipeline<DynamicPipelineKind::Compute> spawnPipeline_;

		DynamicPipeline<DynamicPipelineKind::Compute> simulatePipeline_;

		DynamicPipeline<DynamicPipelineKind::Graphics> quadPipeline_;

		ZephyrModuleAssembler assembler_;

		DynamicArray<ParticleEmitterStructuredBuffer> emitters_;

		DynamicArray<Vector4> parameters_;

		DynamicArray<Uint32> seedCounts_;

		DynamicArray<Uint32> aliveCounts_;

		DynamicArray<EntityID> entities_;

		Bool active_ = false;

		Bool reset_ = false;

		Bool flip_ = false;
	};
}