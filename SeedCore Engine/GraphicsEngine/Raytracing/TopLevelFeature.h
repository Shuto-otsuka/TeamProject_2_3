#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelShader.h>

namespace SeedCore
{
	class TopLevelFeature :public RenderFeature<TopLevelFeature>
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 131072;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Structure, RenderResourceDesc::AccelerationStructure(capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Hits, RenderResourceDesc::FixedBuffer(sizeof(TopLevelHitStructuredBuffer), capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(Instances, RenderResourceDesc::FixedBuffer(sizeof(TopLevelInstanceStructuredBuffer), capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(Counts, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 1), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Descs, RenderResourceDesc::FixedBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient)
		);

	public:
		struct UpdateStage;

		struct BuildStage;

	public:
		void Create(RenderDevice& device);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<TopLevelUpdateDescription> updatePipeline_;

		ComputePipeline<TopLevelResolveDescription> resolvePipeline_;

		Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;

		Uint32 count_ = 0;
	};
}
