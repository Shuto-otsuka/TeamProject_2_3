#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Texture/TextureShader.h>
#include <GraphicsEngine/Texture/TextureUnit.h>

namespace SeedCore
{
	class TextureFeature :public RenderFeature<TextureFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Units, RenderResourceDesc::FixedBuffer(sizeof(TextureUnitStructuredBuffer), TextureUnit::unitCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Quads, RenderResourceDesc::FixedBuffer(sizeof(TextureQuadStructuredBuffer), TextureUnit::quadCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(DrawList, RenderResourceDesc::FixedBuffer(sizeof(Uint32), TextureUnit::quadCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(Constants, RenderResourceDesc::Constant(sizeof(TextureConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(UnitUpdates, RenderResourceDesc::FixedBuffer(sizeof(TextureUnitUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(QuadUpdates, RenderResourceDesc::FixedBuffer(sizeof(TextureQuadUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct UpdateStage;

		struct ExpandStage;

		struct SpriteStage;

		struct BillboardStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<TextureUpdateDescription> updatePipeline_;

		ComputePipeline<TextureExpandDescription> expandPipeline_;

		GraphicsPipeline<TextureSpriteDescription> spritePipeline_;

		GraphicsPipeline<TextureBillboardDescription> billboardPipeline_;

		Bool updating_ = false;

		Bool spriting_ = false;

		Bool billboarding_ = false;

		TextureUnit units_;
	};
}
