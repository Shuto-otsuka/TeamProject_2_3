#pragma once
#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct AnamorphicFlareFeature::PrefilterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Ping)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::Blur1Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Pong)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::Blur2Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Pong),
			RenderAccess::Write(Ping)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::Blur3Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Pong)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::Blur4Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Pong),
			RenderAccess::Write(Ping)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Radiance)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};
}
