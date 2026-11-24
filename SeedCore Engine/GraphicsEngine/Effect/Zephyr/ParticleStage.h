#pragma once
#include <GraphicsEngine/Effect/Zephyr/ParticleFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>

namespace SeedCore
{
	struct ParticleFeature::UpdateStage
	{
		struct ResetStep;

		struct BeginStep;

		struct SpawnStep;

		struct SimulateStep;

		struct DrivenStep;

		using Sequence = RenderSequence<ResetStep, BeginStep, SpawnStep, SimulateStep, DrivenStep>;
	};

	struct ParticleFeature::UpdateStage::ResetStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::reset_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Emitters),
			RenderAccess::Write(Dead),
			RenderAccess::Write(Counters)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};

	struct ParticleFeature::UpdateStage::BeginStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Counters),
			RenderAccess::Write(Offsets),
			RenderAccess::Write(SimulateArguments),
			RenderAccess::Read(Groups)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};

	struct ParticleFeature::UpdateStage::SpawnStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Emitters),
			RenderAccess::Read(Parameters),
			RenderAccess::Write(Seeds),
			RenderAccess::Write(Dead),
			RenderAccess::Write(Alive),
			RenderAccess::Write(Counters)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};

	struct ParticleFeature::UpdateStage::SimulateStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Emitters),
			RenderAccess::Read(Parameters),
			RenderAccess::Write(Seeds),
			RenderAccess::Write(Dead),
			RenderAccess::Write(Alive),
			RenderAccess::Write(Counters),
			RenderAccess::Read(Offsets),
			RenderAccess::Read(SimulateArguments),
			RenderAccess::Read(Groups)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};

	struct ParticleFeature::UpdateStage::DrivenStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Counters),
			RenderAccess::Write(QuadOffsets),
			RenderAccess::Write(QuadArguments),
			RenderAccess::Write(QuadFallbackArguments)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};

	struct ParticleFeature::DrawStage
	{
		struct QuadStep;

		using Sequence = RenderSequence<QuadStep>;
	};

	struct ParticleFeature::DrawStage::QuadStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Effect;

		SC_CONSTEX Bool ParticleFeature::* enableCondition_ = &ParticleFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Emitters),
			RenderAccess::Read(Seeds),
			RenderAccess::Read(Alive),
			RenderAccess::Read(Counters),
			RenderAccess::Read(QuadOffsets),
			RenderAccess::Read(QuadArguments),
			RenderAccess::Read(QuadFallbackArguments),
			RenderAccess::Target(RadianceFeature::Color),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(ParticleFeature& feature, RenderPass& pass);
	};
}
