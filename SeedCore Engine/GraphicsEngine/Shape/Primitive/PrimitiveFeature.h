#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveShader.h>
#include <GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolid.h>
#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.h>

namespace SeedCore
{
	class PrimitiveFeature :public RenderFeature<PrimitiveFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Solids, RenderResourceDesc::FixedBuffer(sizeof(PrimitiveSolidStructuredBuffer), PrimitiveSolid::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Materials, RenderResourceDesc::FixedBuffer(sizeof(ModelMaterialStructuredBuffer), PrimitiveSolid::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(SolidUpdates, RenderResourceDesc::FixedBuffer(sizeof(PrimitiveSolidUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(MaterialUpdates, RenderResourceDesc::FixedBuffer(sizeof(PrimitiveMaterialUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(PrimitiveSolidConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Wireframes, RenderResourceDesc::FixedBuffer(sizeof(PrimitiveWireframeStructuredBuffer), 1), RenderResourceScope::View, RenderResourceLifetime::Upload)
		);

	public:
		struct UpdateStage;

		struct SubmitStage;

		struct DrawStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<PrimitiveSolidUpdateDescription> updatePipeline_;

		ComputePipeline<PrimitiveSolidExpandDescription> expandPipeline_;

		ComputePipeline<PrimitiveSolidRaytracingDescription> raytracingPipeline_;

		GraphicsPipeline<PrimitiveWireframeDescription> wireframePipeline_;

		GraphicsPipeline<PrimitiveCanvasDescription> canvasPipeline_;

		PrimitiveSolid solid_;

		PrimitiveWireframe wireframe_;

		Bool updating_ = false;

		Bool submitting_ = false;

		Bool drawing_[RenderViewCount] = {};
	};
}
