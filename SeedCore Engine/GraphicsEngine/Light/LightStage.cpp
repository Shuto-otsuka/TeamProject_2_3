#include <GraphicsEngine/Light/LightStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void LightFeature::BuildStage::Execute(LightFeature& feature, RenderPass& pass)
	{
		Uint32 tileCountX = (pass.Width() + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
		Uint32 tileCountY = (pass.Height() + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
		feature.clusterPipeline_.Dispatch(pass, tileCountX * tileCountY * LIGHT_CLUSTER_DEPTH_SLICES, 1, 1);
	}
}
