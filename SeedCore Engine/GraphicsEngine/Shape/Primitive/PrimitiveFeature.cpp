#include <GraphicsEngine/Shape/Primitive/PrimitiveFeature.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveStage.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(PrimitiveFeature, UpdateStage, SubmitStage, DrawStage);

	void PrimitiveFeature::Create(RenderDevice& device)
	{
		updatePipeline_.Create(device);
		expandPipeline_.Create(device);
		raytracingPipeline_.Create(device);
		wireframePipeline_.Create(device);
		canvasPipeline_.Create(device);
		solid_.Create(device.Context().GetDevice(), device.ResourceHeap());
	}

	void PrimitiveFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		const ShapeList* found = scene.Find<ShapeList>();
		ShapeList list = found ? *found : ShapeList{};
		solid_.Gather(list, *scene.resource_->textureStreaming_, frame);
		wireframe_.Gather(list);

		updating_ = !solid_.Updates().empty() || !solid_.Materials().empty();
		submitting_ = solid_.Count() != 0;
		for (Uint32 view = 0;view < RenderViewCount;++view)
		{
			drawing_[view] = !wireframe_.Instances(static_cast<RenderViewKind>(view)).empty();
		}
	}

	void PrimitiveFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!solid_.Updates().empty())
		{
			upload.Write(SolidUpdates, RenderViewKind::Game, solid_.Updates());
		}
		if (!solid_.Materials().empty())
		{
			upload.Write(MaterialUpdates, RenderViewKind::Game, solid_.Materials());
		}
		upload.Write(Constants, RenderViewKind::Game, std::span<const PrimitiveSolidConstantBuffer>(&solid_.Constants(), 1));
		for (Uint32 view = 0;view < RenderViewCount;++view)
		{
			std::span<const PrimitiveWireframeStructuredBuffer> instances = wireframe_.Instances(static_cast<RenderViewKind>(view));
			if (!instances.empty())
			{
				upload.Write(Wireframes, static_cast<RenderViewKind>(view), instances);
			}
		}
	}
}
