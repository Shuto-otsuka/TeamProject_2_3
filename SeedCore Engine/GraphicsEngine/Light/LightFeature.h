#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Light/Light.h>
#include <GraphicsEngine/Light/LightCluster.h>
#include <GraphicsEngine/Light/LightShader.h>

namespace SeedCore
{
	class LightFeature :public RenderFeature<LightFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(List, RenderResourceDesc::FixedBuffer(sizeof(LightStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(Constants, RenderResourceDesc::Constant(sizeof(LightConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(ClusterGrid, RenderResourceDesc::ScaledBuffer(sizeof(Uint32), 1 + LIGHT_CLUSTER_MAX_LIGHTS, RenderResourceScale::Render, LIGHT_CLUSTER_TILE_SIZE, LIGHT_CLUSTER_DEPTH_SLICES), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct BuildStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<LightClusterDescription> clusterPipeline_;

		DynamicArray<LightStructuredBuffer> lights_;

		LightConstantBuffer constants_;
	};
}
