#pragma once
#include <GraphicsEngine/Model/ModelFeature.h>

namespace SeedCore
{
	struct ModelFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool ModelFeature::* enableCondition_ = &ModelFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UnitUpdates),
			RenderAccess::Read(MeshUpdates),
			RenderAccess::Read(ClusterUpdates),
			RenderAccess::Read(PlacementUpdates),
			RenderAccess::Read(MaterialUpdates),
			RenderAccess::Write(Units),
			RenderAccess::Write(Meshes),
			RenderAccess::Write(Clusters),
			RenderAccess::Write(Placements),
			RenderAccess::Write(Materials)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};

	struct ModelFeature::ExpandStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Meshes),
			RenderAccess::Read(Clusters),
			RenderAccess::Read(Placements),
			RenderAccess::Read(Materials),
			RenderAccess::Read(SoftbodyInstances),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Instances),
			RenderAccess::Write(Counts)
		};

		static void Execute(ModelFeature& feature, RenderPass& pass);
	};
}
