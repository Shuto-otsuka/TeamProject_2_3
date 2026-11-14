#pragma once
#include <GraphicsEngine/Culling/CullingFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct CullingFeature::CullingStage
	{
		struct EarlyStep;

		struct LateStep;

		struct TransparentStep;

		using Sequence = RenderSequence<EarlyStep, LateStep, TransparentStep>;
	};

	struct CullingFeature::CullingStage::EarlyStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Geometry;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::History(GeometryFeature::HiZ),
			RenderAccess::Write(OccludedMeshlets),
			RenderAccess::Write(OpaqueMeshlets),
			RenderAccess::Write(MeshletArguments),
			RenderAccess::Write(TransparentArguments),
			RenderAccess::Write(OpaqueChunks),
			RenderAccess::Write(OccludedChunks),
			RenderAccess::Write(ChunkArguments),
			RenderAccess::Write(GeometryFeature::OccludedMeshlets)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};

	struct CullingFeature::CullingStage::LateStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Occlusion;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::Read(GeometryFeature::HiZ),
			RenderAccess::Read(OccludedMeshlets),
			RenderAccess::Write(OpaqueMeshlets),
			RenderAccess::Write(MeshletArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};

	struct CullingFeature::CullingStage::TransparentStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Transparent);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Counts),
			RenderAccess::Read(GeometryFeature::HiZ),
			RenderAccess::Write(TransparentMeshlets),
			RenderAccess::Write(TransparentChunks),
			RenderAccess::Write(TransparentArguments)
		};

		static void Execute(CullingFeature& feature, RenderPass& pass);
	};
}
