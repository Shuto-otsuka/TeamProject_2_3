#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Preview/PreviewShader.h>
#include <GraphicsEngine/Model/ModelDriven.h>
#include <GraphicsEngine/Model/Material/ModelMaterial.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	class Crister;

	class PreviewFeature :public RenderFeature<PreviewFeature>
	{
	public:
		SC_CONSTEX RenderFeatureSet set_ = RenderFeatureSet::Preview;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Instances, RenderResourceDesc::FixedBuffer(sizeof(ModelStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Meshlets, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Bones, RenderResourceDesc::FixedBuffer(sizeof(Matrix), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Lines, RenderResourceDesc::FixedBuffer(sizeof(PreviewLineStructuredBuffer), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Depth, RenderResourceDesc::ScaledTexture(RenderFormat::D32Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(Output, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA8Unorm, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Imported)
		);

	public:
		struct DrawStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		GraphicsPipeline<PreviewMeshDescription> meshPipeline_;

		GraphicsPipeline<PreviewLineDescription> linePipeline_;

		ModelMaterial materials_;

		DynamicArray<ModelStructuredBuffer> instances_;

		DynamicArray<Uint32> meshlets_;

		DynamicArray<Matrix> bones_;

		DynamicArray<PreviewLineStructuredBuffer> lines_;

		Handle<Crister> crister_;

		Uint32 surfaceAssetID_ = 0;

		Uint64 frame_ = 0;

		Bool lining_ = false;
	};
}
