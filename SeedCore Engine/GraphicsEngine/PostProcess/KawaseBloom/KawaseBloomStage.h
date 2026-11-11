#pragma once
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct KawaseBloomFeature::PrefilterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Radiance)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Downsample1Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Radiance),
			RenderAccess::Write(Level1)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Downsample2Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Level1),
			RenderAccess::Write(Level2)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Downsample3Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Level2),
			RenderAccess::Write(Level3)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Downsample4Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Level3),
			RenderAccess::Write(Level4)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Downsample5Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Level4),
			RenderAccess::Write(Level5)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Upsample4Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Level5),
			RenderAccess::Write(Level4)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Upsample3Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Level4),
			RenderAccess::Write(Level3)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Upsample2Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Level3),
			RenderAccess::Write(Level2)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Upsample1Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Level2),
			RenderAccess::Write(Level1)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::Upsample0Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Level1),
			RenderAccess::Write(Radiance)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};
}
