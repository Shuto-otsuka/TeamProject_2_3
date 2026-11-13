#include <GraphicsEngine/Culling/CullingFeature.h>
#include <GraphicsEngine/Culling/CullingStage.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(CullingFeature, CullingStage);

	void CullingFeature::Create(RenderDevice& device)
	{
		mesh_ = device.Level() == D3D12Level::D12_2;
		preparePipeline_.Create(device);
		earlyPipeline_.Create(device);
		latePipeline_.Create(device);
		transparentPipeline_.Create(device);
	}
}
